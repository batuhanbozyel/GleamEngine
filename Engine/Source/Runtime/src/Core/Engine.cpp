#include "gpch.h"
#include "Engine.h"

#include "EventSystem.h"
#include "WindowSystem.h"
#include "IO/FileWatcher.h"
#include "Input/InputSystem.h"
#include "World/ScriptingSystem.h"
#include "Renderer/RenderSystem.h"
#include "Renderer/Renderers/WorldRenderer.h"
#include "Renderer/Renderers/SkyAtmosphere.h"
#include "Renderer/Renderers/PostProcessStack.h"
#include "Serialization/JSONSerializer.h"
#include "Serialization/BinarySerializer.h"

using namespace Gleam;

void Engine::Initialize(const CommandLine& cli)
{
	int logLevel = cli("-log-level", static_cast<int>(Logger::Level::Info));
	Logger::SetLevel(static_cast<Logger::Level>(logLevel));

	// init serialization
	AddSubsystem<BinarySerializer>();
	AddSubsystem<JSONSerializer>();

	// init core subsystems
	AddSubsystem<EventSystem>();
	AddSubsystem<InputSystem>();
    AddSubsystem<FileWatcher>();
	AddSubsystem<ScriptingSystem>();

	// subscribe to window resize
	EventDispatcher<WindowResizeEvent>::Subscribe([this](WindowResizeEvent e)
	{
		mConfig.window.size = Size(static_cast<float>(e.GetWidth()),
								   static_cast<float>(e.GetHeight()));
		SaveConfigToDisk();
	});

	// setup config
	Globals::StartupDirectory = Filesystem::WorkingDirectory();
	Globals::BuiltinAssetsDirectory = Globals::StartupDirectory / "Assets";

	auto configFile = Globals::StartupDirectory/"Engine.config";
	if (Filesystem::Exists(configFile))
	{
		auto file = Filesystem::Open(configFile, FileType::Text);
		auto accessor = Filesystem::ReadAccessor(configFile);
        auto serializer = JSONSerializer();
		mConfig = serializer.Deserialize<EngineConfig>(file.GetStream());
	}
	
	// init windowing subsystem
	auto windowSubsystem = AddSubsystem<WindowSystem>();
	windowSubsystem->Configure(mConfig.window);
	
	// init renderer backend
	auto renderSubsystem = AddSubsystem<RenderSystem>();
	renderSubsystem->Configure(mConfig.renderer);

	// add default renderers
	renderSubsystem->AddRenderer<WorldRenderer>();
	renderSubsystem->AddRenderer<SkyAtmosphere>();
	renderSubsystem->AddRenderer<PostProcessStack>();
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
    auto file = Filesystem::Create(Globals::StartupDirectory/"Engine.config", FileType::Text);
	auto accessor = Filesystem::WriteAccessor(Globals::StartupDirectory / "Engine.config");
    auto serializer = JSONSerializer();
    serializer.Serialize(mConfig, file.GetStream());
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
