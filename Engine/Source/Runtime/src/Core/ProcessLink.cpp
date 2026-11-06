#include "gpch.h"
#include "ProcessLink.h"
#include "Application.h"
#include "EventSystem.h"
#include "Events/ApplicationEvent.h"

#include "Networking/IPCChannel.h"
#include "World/World.h"
#include "World/WorldManager.h"
#include "Physics/PhysicsSystem.h"

using namespace Gleam;

TString ProcessLink::ChannelName(uint32_t processId)
{
	return fmt::format("Gleam.Link.{}", processId);
}

ProcessLink::ProcessLink(uint32_t processId)
	: mProcessId(processId)
{

}

void ProcessLink::Initialize(Application* app)
{
	mChannel = IPCChannel::Connect(ChannelName(mProcessId));
}

void ProcessLink::Shutdown(Application* app)
{
	delete mChannel;
	mChannel = nullptr;
}

void ProcessLink::Tick(Application* app)
{
	World* world = app->HasSubsystem<WorldManager>() ? app->GetSubsystem<WorldManager>()->GetActiveWorld() : nullptr;

	IPCMessage message;
	while (mChannel and mChannel->Receive(message))
	{
		switch (static_cast<ProcessLinkMessage>(message.type))
		{
			case ProcessLinkMessage::Quit:
			{
				EventDispatcher<AppCloseEvent>::Publish(AppCloseEvent());
				break;
			}
			case ProcessLinkMessage::Pause:
			case ProcessLinkMessage::Resume:
			{
				if (world)
				{
					world->GetSystem<PhysicsSystem>()->Enabled = static_cast<ProcessLinkMessage>(message.type) == ProcessLinkMessage::Resume;
				}
				break;
			}
			case ProcessLinkMessage::Step:
			{
				if (world)
				{
					world->GetSystem<PhysicsSystem>()->OnFixedUpdate(world->GetEntityManager());
				}
				break;
			}
			default:
			{
				break;
			}
		}
	}

	if (mChannel == nullptr or mChannel->IsClosed())
	{
		EventDispatcher<AppCloseEvent>::Publish(AppCloseEvent());
	}
}
