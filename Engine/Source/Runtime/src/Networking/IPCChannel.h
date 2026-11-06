#pragma once
#include "Core/Macro.h"
#include "Container/Array.h"
#include "Container/String.h"

namespace Gleam {

struct IPCMessage
{
	uint32_t type = 0;
	TArray<uint8_t> payload;
};

class IPCChannel final
{
public:

	GLEAM_NONCOPYABLE(IPCChannel);

	static IPCChannel* Listen(const TString& name);

	static IPCChannel* Connect(const TString& name);

	~IPCChannel();

	bool IsConnected();

	bool IsClosed() const
	{
		return mClosed;
	}

	bool Send(uint32_t type, const void* data = nullptr, uint32_t size = 0);

	bool Receive(IPCMessage& message);

private:

	IPCChannel() = default;

	bool Accept();

	bool Write(const void* data, size_t size);

	bool ReadAvailable();

	void Close();

	intptr_t mListener = -1;

	intptr_t mConnection = -1;

	TString mEndpoint;

	TArray<uint8_t> mReceiveBuffer;

	bool mClosed = false;

};

} // namespace Gleam
