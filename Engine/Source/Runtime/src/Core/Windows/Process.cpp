#include "gpch.h"

#ifdef PLATFORM_WINDOWS
#include "Core/Process.h"

#include <Windows.h>
#include <shellapi.h>
#include <objbase.h>
#include <oleauto.h>

#include <csignal>
#include <cstdlib>

using namespace Gleam;

// Exception filters take no context, so everything the relaunch needs is prepared up front
class CrashRelaunch final
{
public:

	static void Install(const Path& crashLog, const TArray<TString>& arguments)
	{
		mExecutable = Process::ExecutablePath().Native();
		mLogFile = (Filesystem::WorkingDirectory() / Logger::FILE_NAME).Native();
		mCrashLogFile = crashLog.Native();

		mCommandLine = L"\"" + mExecutable + L"\"";
		for (const auto& argument : arguments)
		{
			mCommandLine += L" \"";
			mCommandLine.append_convert(argument);
			mCommandLine += L"\"";
		}

		SetUnhandledExceptionFilter(HandleException);
		_set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
		std::signal(SIGABRT, HandleAbort);
	}

private:

	static LONG WINAPI HandleException(EXCEPTION_POINTERS* exception)
	{
		Relaunch("unhandled exception");
		return EXCEPTION_EXECUTE_HANDLER;
	}

	static void HandleAbort(int signal)
	{
		Relaunch("abort");
		TerminateProcess(GetCurrentProcess(), 3);
	}

	static void Relaunch(const char* reason)
	{
		// The log stays open for writing, so it is copied with a shared read instead of moved
		const auto source = CreateFileW(mLogFile.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, 0, nullptr);
		const auto target = CreateFileW(mCrashLogFile.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, 0, nullptr);
		if (target != INVALID_HANDLE_VALUE)
		{
			DWORD bytesWritten = 0;
			if (source != INVALID_HANDLE_VALUE)
			{
				char buffer[4096];
				DWORD bytesRead = 0;
				while (ReadFile(source, buffer, sizeof(buffer), &bytesRead, nullptr) and bytesRead > 0)
				{
					WriteFile(target, buffer, bytesRead, &bytesWritten, nullptr);
				}
				CloseHandle(source);
			}

			constexpr char prefix[] = "[fatal] GLEAM: Crashed with ";
			WriteFile(target, prefix, sizeof(prefix) - 1, &bytesWritten, nullptr);
			WriteFile(target, reason, static_cast<DWORD>(strlen(reason)), &bytesWritten, nullptr);
			WriteFile(target, "\n", 1, &bytesWritten, nullptr);
			CloseHandle(target);
		}

		STARTUPINFOW startupInfo = {};
		startupInfo.cb = sizeof(STARTUPINFOW);
		PROCESS_INFORMATION processInfo = {};
		if (CreateProcessW(mExecutable.c_str(), mCommandLine.data(), nullptr, nullptr, FALSE, 0, nullptr, nullptr, &startupInfo, &processInfo))
		{
			CloseHandle(processInfo.hThread);
			CloseHandle(processInfo.hProcess);
		}
	}

	static inline TWString mExecutable;
	static inline TWString mLogFile;
	static inline TWString mCrashLogFile;
	static inline TWString mCommandLine;
};

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

void Process::OpenWithDefaultApplication(const Path& path)
{
	ShellExecuteW(nullptr, L"open", path.Native().c_str(), nullptr, nullptr, SW_SHOWNORMAL);
}

void Process::RelaunchOnCrash(const Path& crashLog, const TArray<TString>& arguments)
{
	CrashRelaunch::Install(crashLog, arguments);
}

#endif
