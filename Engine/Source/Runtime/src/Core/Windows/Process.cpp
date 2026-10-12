#include "gpch.h"

#ifdef PLATFORM_WINDOWS
#include "Core/Process.h"

#include <Windows.h>
#include <shellapi.h>
#include <objbase.h>
#include <oleauto.h>

using namespace Gleam;

static HRESULT InvokeMember(IDispatch* object, const wchar_t* name, WORD flags, VARIANT* arguments, UINT argumentCount, VARIANT* result)
{
	constexpr int kMaxAttempts = 50;

	DISPID dispatchId = DISPID_UNKNOWN;
	LPOLESTR memberName = const_cast<LPOLESTR>(name);
	HRESULT hr = E_FAIL;
	for (int attempt = 0; attempt < kMaxAttempts; ++attempt)
	{
		hr = object->GetIDsOfNames(IID_NULL, &memberName, 1, LOCALE_USER_DEFAULT, &dispatchId);
		if (hr != RPC_E_CALL_REJECTED and hr != RPC_E_SERVERCALL_RETRYLATER)
		{
			break;
		}
		Sleep(100);
	}

	if (FAILED(hr))
	{
		return hr;
	}

	DISPPARAMS parameters = { arguments, nullptr, argumentCount, 0 };
	for (int attempt = 0; attempt < kMaxAttempts; ++attempt)
	{
		hr = object->Invoke(dispatchId, IID_NULL, LOCALE_USER_DEFAULT, flags, &parameters, result, nullptr, nullptr);
		if (hr != RPC_E_CALL_REJECTED and hr != RPC_E_SERVERCALL_RETRYLATER)
		{
			break;
		}
		Sleep(100);
	}
	return hr;
}

static IDispatch* GetDispatchProperty(IDispatch* object, const wchar_t* name)
{
	VARIANT result;
	VariantInit(&result);
	if (SUCCEEDED(InvokeMember(object, name, DISPATCH_PROPERTYGET, nullptr, 0, &result)) and result.vt == VT_DISPATCH)
	{
		return result.pdispVal;
	}
	VariantClear(&result);
	return nullptr;
}

static long GetLongProperty(IDispatch* object, const wchar_t* name)
{
	VARIANT result;
	VariantInit(&result);
	long value = -1;
	if (SUCCEEDED(InvokeMember(object, name, DISPATCH_PROPERTYGET, nullptr, 0, &result)) and SUCCEEDED(VariantChangeType(&result, &result, 0, VT_I4)))
	{
		value = result.lVal;
	}
	VariantClear(&result);
	return value;
}

static IDispatch* FindProcess(IDispatch* processes, DWORD processId)
{
	const long count = GetLongProperty(processes, L"Count");
	for (long index = 1; index <= count; ++index)
	{
		VARIANT argument;
		VariantInit(&argument);
		argument.vt = VT_I4;
		argument.lVal = index;

		VARIANT item;
		VariantInit(&item);
		if (SUCCEEDED(InvokeMember(processes, L"Item", DISPATCH_METHOD | DISPATCH_PROPERTYGET, &argument, 1, &item)) and item.vt == VT_DISPATCH)
		{
			if (GetLongProperty(item.pdispVal, L"ProcessID") == static_cast<long>(processId))
			{
				return item.pdispVal;
			}
		}
		VariantClear(&item);
	}
	return nullptr;
}

static IDispatch* FindDebuggerOfCurrentProcess()
{
	IRunningObjectTable* runningObjects = nullptr;
	if (FAILED(GetRunningObjectTable(0, &runningObjects)))
	{
		return nullptr;
	}

	IEnumMoniker* monikers = nullptr;
	IBindCtx* bindContext = nullptr;
	IDispatch* found = nullptr;
	if (SUCCEEDED(runningObjects->EnumRunning(&monikers)) and SUCCEEDED(CreateBindCtx(0, &bindContext)))
	{
		IMoniker* moniker = nullptr;
		while (found == nullptr and monikers->Next(1, &moniker, nullptr) == S_OK)
		{
			LPOLESTR displayName = nullptr;
			if (SUCCEEDED(moniker->GetDisplayName(bindContext, nullptr, &displayName)))
			{
				IUnknown* object = nullptr;
				if (wcsncmp(displayName, L"!VisualStudio.DTE.", 18) == 0 and SUCCEEDED(runningObjects->GetObject(moniker, &object)))
				{
					IDispatch* dte = nullptr;
					if (SUCCEEDED(object->QueryInterface(IID_IDispatch, reinterpret_cast<void**>(&dte))))
					{
						if (IDispatch* debugger = GetDispatchProperty(dte, L"Debugger"))
						{
							if (IDispatch* debuggedProcesses = GetDispatchProperty(debugger, L"DebuggedProcesses"))
							{
								if (IDispatch* process = FindProcess(debuggedProcesses, GetCurrentProcessId()))
								{
									process->Release();
									found = debugger;
								}
								debuggedProcesses->Release();
							}

							if (found == nullptr)
							{
								debugger->Release();
							}
						}
						dte->Release();
					}
					object->Release();
				}
				CoTaskMemFree(displayName);
			}
			moniker->Release();
		}
	}

	if (bindContext)
	{
		bindContext->Release();
	}
	if (monikers)
	{
		monikers->Release();
	}
	runningObjects->Release();
	return found;
}

static bool AttachDebugger(DWORD processId)
{
	const HRESULT initialization = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

	bool attached = false;
	if (IDispatch* debugger = FindDebuggerOfCurrentProcess())
	{
		if (IDispatch* localProcesses = GetDispatchProperty(debugger, L"LocalProcesses"))
		{
			if (IDispatch* process = FindProcess(localProcesses, processId))
			{
				VARIANT result;
				VariantInit(&result);
				attached = SUCCEEDED(InvokeMember(process, L"Attach", DISPATCH_METHOD, nullptr, 0, &result));
				VariantClear(&result);
				process->Release();
			}
			localProcesses->Release();
		}
		debugger->Release();
	}

	if (SUCCEEDED(initialization))
	{
		CoUninitialize();
	}
	return attached;
}

bool Process::Launch(const Path& executable, const TArray<TString>& arguments, const Path& workingDirectory)
{
	TWString commandLine = L"\"" + executable.Native() + L"\"";
	for (const auto& argument : arguments)
	{
		commandLine += L" \"";
		commandLine.append_convert(argument);
		commandLine += L"\"";
	}

	STARTUPINFOW startupInfo = {};
	startupInfo.cb = sizeof(STARTUPINFOW);
	PROCESS_INFORMATION processInfo = {};
	if (CreateProcessW(executable.Native().c_str(), commandLine.data(), nullptr, nullptr, FALSE, 0, nullptr, workingDirectory.Native().c_str(), &startupInfo, &processInfo) == FALSE)
	{
		return false;
	}

	if (IsDebuggerPresent() and AttachDebugger(processInfo.dwProcessId) == false)
	{
		GLEAM_CORE_WARN("Debugger could not be attached to launched process: {0}", executable.String());
	}

	CloseHandle(processInfo.hThread);
	CloseHandle(processInfo.hProcess);
	return true;
}

Path Process::ExecutablePath()
{
	TWString buffer(MAX_PATH, L'\0');
	DWORD length = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
	while (length == buffer.size())
	{
		buffer.resize(buffer.size() * 2);
		length = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
	}
	buffer.resize(length);
	return Path(buffer);
}

void Process::RevealInFileBrowser(const Path& path)
{
	TWString nativePath = path.Native();
	eastl::replace(nativePath.begin(), nativePath.end(), L'/', L'\\');

	TWString parameters = L"/select,\"" + nativePath + L"\"";
	ShellExecuteW(nullptr, L"open", L"explorer.exe", parameters.c_str(), nullptr, SW_SHOWNORMAL);
}

#endif
