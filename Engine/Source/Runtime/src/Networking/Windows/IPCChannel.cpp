#include "gpch.h"

#ifdef PLATFORM_WINDOWS
#include "Networking/IPCChannel.h"

using namespace Gleam;

static constexpr DWORD kPipeBufferSize = 64 * 1024;

static TWString PipeName(const TString& name)
{
	TWString pipeName = L"\\\\.\\pipe\\";
	pipeName.append_convert(name);
	return pipeName;
}

IPCChannel* IPCChannel::Listen(const TString& name)
{
	const auto pipe = ::CreateNamedPipeW(PipeName(name).c_str(), PIPE_ACCESS_DUPLEX | FILE_FLAG_FIRST_PIPE_INSTANCE, PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_NOWAIT | PIPE_REJECT_REMOTE_CLIENTS, 1, kPipeBufferSize, kPipeBufferSize, 0, nullptr);
	if (pipe == INVALID_HANDLE_VALUE)
	{
		GLEAM_CORE_ERROR("IPC: Pipe could not be created: {0}", name);
		return nullptr;
	}

	auto channel = new IPCChannel();
	channel->mListener = reinterpret_cast<intptr_t>(pipe);
	return channel;
}

IPCChannel* IPCChannel::Connect(const TString& name)
{
	const auto pipeName = PipeName(name);
	auto pipe = ::CreateFileW(pipeName.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
	if (pipe == INVALID_HANDLE_VALUE and ::GetLastError() == ERROR_PIPE_BUSY and ::WaitNamedPipeW(pipeName.c_str(), 2000))
	{
		pipe = ::CreateFileW(pipeName.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
	}

	if (pipe == INVALID_HANDLE_VALUE)
	{
		GLEAM_CORE_ERROR("IPC: Pipe could not be connected: {0}", name);
		return nullptr;
	}

	auto channel = new IPCChannel();
	channel->mConnection = reinterpret_cast<intptr_t>(pipe);
	return channel;
}

bool IPCChannel::Accept()
{
	bool accepted = false;
	if (mListener != -1)
	{
		const auto pipe = reinterpret_cast<HANDLE>(mListener);
		const bool listening = ::ConnectNamedPipe(pipe, nullptr) or ::GetLastError() == ERROR_PIPE_LISTENING;
		if (listening == false and ::GetLastError() == ERROR_PIPE_CONNECTED)
		{
			DWORD mode = PIPE_READMODE_BYTE | PIPE_WAIT;
			::SetNamedPipeHandleState(pipe, &mode, nullptr, nullptr);
			mConnection = mListener;
			mListener = -1;
			accepted = true;
		}
		else if (listening == false)
		{
			Close();
		}
	}
	return accepted;
}

bool IPCChannel::Write(const void* data, size_t size)
{
	DWORD written = 0;
	return ::WriteFile(reinterpret_cast<HANDLE>(mConnection), data, static_cast<DWORD>(size), &written, nullptr) and written == size;
}

bool IPCChannel::ReadAvailable()
{
	const auto pipe = reinterpret_cast<HANDLE>(mConnection);
	DWORD available = 0;
	if (::PeekNamedPipe(pipe, nullptr, 0, nullptr, &available, nullptr) == FALSE)
	{
		return false;
	}

	bool success = true;
	if (available > 0)
	{
		const auto offset = mReceiveBuffer.size();
		mReceiveBuffer.resize(offset + available);
		DWORD read = 0;
		success = ::ReadFile(pipe, mReceiveBuffer.data() + offset, available, &read, nullptr);
		mReceiveBuffer.resize(offset + read);
	}
	return success;
}

void IPCChannel::Close()
{
	if (mConnection != -1)
	{
		::CloseHandle(reinterpret_cast<HANDLE>(mConnection));
		mConnection = -1;
	}
	if (mListener != -1)
	{
		::CloseHandle(reinterpret_cast<HANDLE>(mListener));
		mListener = -1;
	}
	mClosed = true;
}

#endif
