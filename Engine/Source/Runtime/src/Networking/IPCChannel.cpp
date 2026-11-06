#include "gpch.h"
#include "IPCChannel.h"

using namespace Gleam;

struct IPCMessageHeader
{
	uint32_t type = 0;
	uint32_t size = 0;
};

IPCChannel::~IPCChannel()
{
	Close();
}

bool IPCChannel::IsConnected()
{
	if (mConnection == -1 and mClosed == false)
	{
		Accept();
	}
	return mConnection != -1;
}

bool IPCChannel::Send(uint32_t type, const void* data, uint32_t size)
{
	bool sent = false;
	if (IsConnected())
	{
		const IPCMessageHeader header{ .type = type, .size = size };
		sent = Write(&header, sizeof(header)) and (size == 0 or Write(data, size));
		if (sent == false)
		{
			Close();
		}
	}
	return sent;
}

bool IPCChannel::Receive(IPCMessage& message)
{
	if (IsConnected() and ReadAvailable() == false)
	{
		Close();
	}

	IPCMessageHeader header;
	if (mReceiveBuffer.size() < sizeof(header))
	{
		return false;
	}
	memcpy(&header, mReceiveBuffer.data(), sizeof(header));

	const size_t messageSize = sizeof(header) + header.size;
	if (mReceiveBuffer.size() < messageSize)
	{
		return false;
	}

	message.type = header.type;
	message.payload.assign(mReceiveBuffer.begin() + sizeof(header), mReceiveBuffer.begin() + messageSize);
	mReceiveBuffer.erase(mReceiveBuffer.begin(), mReceiveBuffer.begin() + messageSize);
	return true;
}
