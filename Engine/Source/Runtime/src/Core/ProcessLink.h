#pragma once
#include "Subsystem.h"
#include "Container/String.h"

namespace Gleam {

class IPCChannel;

enum class ProcessLinkMessage : uint32_t
{
	Quit,
	Pause,
	Resume,
	Step
};

class ProcessLink final : public TickableGameInstanceSubsystem
{
public:

	static TString ChannelName(uint32_t processId);

	explicit ProcessLink(uint32_t processId);

	virtual void Initialize(Application* app) override;

	virtual void Shutdown(Application* app) override;

	virtual void Tick(Application* app) override;

private:

	uint32_t mProcessId = 0;

	IPCChannel* mChannel = nullptr;

};

} // namespace Gleam
