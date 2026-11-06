#include "gpch.h"

#ifdef PLATFORM_MACOS
#include "Networking/IPCChannel.h"

#include <sys/socket.h>
#include <sys/un.h>
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>

using namespace Gleam;

static TString SocketPath(const TString& name)
{
	const char* directory = getenv("TMPDIR");
	TString path = directory ? directory : "/tmp/";
	if (path.back() != '/')
	{
		path.push_back('/');
	}
	return path + name + ".sock";
}

static sockaddr_un SocketAddress(const TString& path)
{
	sockaddr_un address = {};
	address.sun_family = AF_UNIX;
	strlcpy(address.sun_path, path.c_str(), sizeof(address.sun_path));
	return address;
}

IPCChannel* IPCChannel::Listen(const TString& name)
{
	const auto path = SocketPath(name);
	const auto address = SocketAddress(path);
	const int listener = socket(AF_UNIX, SOCK_STREAM, 0);
	unlink(path.c_str());
	if (listener == -1 or bind(listener, reinterpret_cast<const sockaddr*>(&address), sizeof(address)) == -1 or listen(listener, 1) == -1)
	{
		GLEAM_CORE_ERROR("IPC: Socket could not be created: {0}", path);
		if (listener != -1)
		{
			close(listener);
		}
		return nullptr;
	}
	fcntl(listener, F_SETFL, fcntl(listener, F_GETFL) | O_NONBLOCK);

	auto channel = new IPCChannel();
	channel->mListener = listener;
	channel->mEndpoint = path;
	return channel;
}

IPCChannel* IPCChannel::Connect(const TString& name)
{
	const auto path = SocketPath(name);
	const auto address = SocketAddress(path);
	const int connection = socket(AF_UNIX, SOCK_STREAM, 0);
	if (connection == -1 or connect(connection, reinterpret_cast<const sockaddr*>(&address), sizeof(address)) == -1)
	{
		GLEAM_CORE_ERROR("IPC: Socket could not be connected: {0}", path);
		if (connection != -1)
		{
			close(connection);
		}
		return nullptr;
	}
	const int enabled = 1;
	setsockopt(connection, SOL_SOCKET, SO_NOSIGPIPE, &enabled, sizeof(enabled));

	auto channel = new IPCChannel();
	channel->mConnection = connection;
	return channel;
}

bool IPCChannel::Accept()
{
	bool accepted = false;
	if (mListener != -1)
	{
		const int connection = accept(static_cast<int>(mListener), nullptr, nullptr);
		if (connection != -1)
		{
			fcntl(connection, F_SETFL, fcntl(connection, F_GETFL) & ~O_NONBLOCK);
			const int enabled = 1;
			setsockopt(connection, SOL_SOCKET, SO_NOSIGPIPE, &enabled, sizeof(enabled));
			close(static_cast<int>(mListener));
			unlink(mEndpoint.c_str());
			mListener = -1;
			mEndpoint.clear();
			mConnection = connection;
			accepted = true;
		}
		else if (errno != EAGAIN and errno != EWOULDBLOCK)
		{
			Close();
		}
	}
	return accepted;
}

bool IPCChannel::Write(const void* data, size_t size)
{
	auto bytes = static_cast<const uint8_t*>(data);
	while (size > 0)
	{
		const auto sent = send(static_cast<int>(mConnection), bytes, size, 0);
		if (sent <= 0)
		{
			return false;
		}
		bytes += sent;
		size -= static_cast<size_t>(sent);
	}
	return true;
}

bool IPCChannel::ReadAvailable()
{
	uint8_t buffer[4096];
	for (;;)
	{
		const auto received = recv(static_cast<int>(mConnection), buffer, sizeof(buffer), MSG_DONTWAIT);
		if (received > 0)
		{
			mReceiveBuffer.insert(mReceiveBuffer.end(), buffer, buffer + received);
		}
		else
		{
			return received == -1 and (errno == EAGAIN or errno == EWOULDBLOCK);
		}
	}
}

void IPCChannel::Close()
{
	if (mConnection != -1)
	{
		close(static_cast<int>(mConnection));
		mConnection = -1;
	}
	if (mListener != -1)
	{
		close(static_cast<int>(mListener));
		mListener = -1;
	}
	if (mEndpoint.empty() == false)
	{
		unlink(mEndpoint.c_str());
		mEndpoint.clear();
	}
	mClosed = true;
}

#endif
