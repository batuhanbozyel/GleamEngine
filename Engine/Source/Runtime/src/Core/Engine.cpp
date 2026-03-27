#include "gpch.h"
#include "Engine.h"

#include "EventSystem.h"
#include "WindowSystem.h"
#include "IO/FileWatcher.h"
#include "Input/InputSystem.h"
#include "World/ScriptingSystem.h"
#include "Renderer/RenderSystem.h"
#include "Serialization/JSONSerializer.h"
#include "Serialization/BinarySerializer.h"

namespace Gleam {

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
		auto file = Filesystem::OpenRead(configFile, FileType::Text);
		auto serializer = JSONSerializer();
		mConfig = serializer.Deserialize<EngineConfig>(file->GetStream());
	}
	
	// init core subsystems
	auto windowSubsystem = AddSubsystem<WindowSystem>();
	auto renderSubsystem = AddSubsystem<RenderSystem>();
}

void Engine::Shutdown()
{
	for (int i = (int)mSubsystems.size() - 1; i >= 0; --i)
	{
		mSubsystems[i]->Shutdown(this);
	}
	mSubsystems.clear();
}

void Engine::SaveConfigToDisk() const
{
	auto file = Filesystem::Create(Globals::StartupDirectory/"Engine.config", FileType::Text);
	auto serializer = JSONSerializer();
	serializer.Serialize(mConfig, file->GetStream());
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

} // namespace Gleam

