#include "gpch.h"
#include "Engine.h"

#include "EventSystem.h"
#include "WindowSystem.h"
#include "ConfigSystem.h"
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
	
	// setup directories
	Globals::StartupDirectory = Filesystem::WorkingDirectory();
	Globals::BuiltinAssetsDirectory = Globals::StartupDirectory / "Assets";

	// init serialization
	AddSubsystem<BinarySerializer>();
	AddSubsystem<JSONSerializer>();
	AddSubsystem<ConfigSystem>();

	// init core subsystems
	AddSubsystem<EventSystem>();
	AddSubsystem<InputSystem>();
	AddSubsystem<FileWatcher>();
	AddSubsystem<WindowSystem>();
	AddSubsystem<RenderSystem>();
	AddSubsystem<ScriptingSystem>();
}

void Engine::Shutdown()
{
	for (int i = (int)mSubsystems.size() - 1; i >= 0; --i)
	{
		mSubsystems[i]->Shutdown(this);
	}
	mSubsystems.clear();
}

Size Engine::GetResolution() const
{
	return GetSubsystem<ConfigSystem>()->Get<WindowConfig>().size;
}

} // namespace Gleam

