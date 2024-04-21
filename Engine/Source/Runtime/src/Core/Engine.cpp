#include "gpch.h"
#include "Engine.h"

#include "EventSystem.h"
#include "WindowSystem.h"
#include "Input/InputSystem.h"
#include "Reflection/Database.h"
#include "Renderer/RenderSystem.h"
#include "Serialization/JSONSerializer.h"

using namespace Gleam;

void Engine::Initialize()
{
    // TODO: load config from config.ini
    
	// init reflection & serialization
	AddSubsystem<Reflection::Database>();
	AddSubsystem<JSONSerializer>();

	// init core subsystems
	AddSubsystem<EventSystem>();
	AddSubsystem<InputSystem>();
	
	// init windowing subsystem
	auto windowSubsystem = AddSubsystem<WindowSystem>();
	windowSubsystem->Configure(mConfig.window);
	
	// init renderer backend
	auto renderSubsystem = AddSubsystem<RenderSystem>();
	renderSubsystem->Configure(mConfig.renderer);
	renderSubsystem->ResetRenderTarget();
    
    EventDispatcher<WindowResizeEvent>::Subscribe([this](WindowResizeEvent e)
    {
        mConfig.window.size = Size(e.GetWidth(), e.GetHeight());
        SaveConfigToDisk();
    });
}

void Engine::Shutdown()
{
	for (auto system : mSubsystems)
	{
		system->Shutdown();
	}
	mSubsystems.clear();
}

void Engine::SaveConfigToDisk()
{
    // TODO: write to config.ini
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
