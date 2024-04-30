#include "gpch.h"
#include "Engine.h"

#include "EventSystem.h"
#include "WindowSystem.h"
#include "Input/InputSystem.h"
#include "Reflection/Database.h"
#include "Renderer/RenderSystem.h"
#include "Serialization/JSONSerializer.h"
#include "Serialization/BinarySerializer.h"

using namespace Gleam;

void Engine::Initialize()
{
	// init reflection & serialization
	AddSubsystem<Reflection::Database>();
	AddSubsystem<BinarySerializer>();
	AddSubsystem<JSONSerializer>();

	// init core subsystems
	AddSubsystem<EventSystem>();
	AddSubsystem<InputSystem>();

	// subscribe to window resize
	EventDispatcher<WindowResizeEvent>::Subscribe([this](WindowResizeEvent e)
	{
		mConfig.window.size = Size(static_cast<float>(e.GetWidth()),
								   static_cast<float>(e.GetHeight()));
		SaveConfigToDisk();
	});

	// setup config
	Globals::StartupDirectory = Filesystem::current_path();
	auto configFile = Globals::StartupDirectory/"Engine.config";
	if (Filesystem::exists(configFile))
	{
		auto file = File(configFile, FileType::Text);
		mConfig = JSONSerializer::Deserialize<EngineConfig>(file.Read());
	}
	
	// init windowing subsystem
	auto windowSubsystem = AddSubsystem<WindowSystem>();
	windowSubsystem->Configure(mConfig.window);
	
	// init renderer backend
	auto renderSubsystem = AddSubsystem<RenderSystem>();
	renderSubsystem->Configure(mConfig.renderer);
	renderSubsystem->ResetRenderTarget();
}

void Engine::Shutdown()
{
	for (auto system : mSubsystems)
	{
		system->Shutdown();
	}
	mSubsystems.clear();
}

void Engine::SaveConfigToDisk() const
{
	auto serialized = JSONSerializer::Serialize(mConfig);
	auto file = File(Globals::StartupDirectory/"Engine.config", FileType::Text);
	file.Write(serialized);
}

void Engine::UpdateConfig(const WindowConfig& config)
{
    mConfig.window = config;
    SaveConfigToDisk();
}

void Engine::UpdateConfig(const RendererConfig& config)
{
    mConfig.renderer = config;
    SaveConfigToDisk();
}

Size Engine::GetResolution() const
{
    return mConfig.window.size;
}

const EngineConfig& Engine::GetConfiguration() const
{
    return mConfig;
}
