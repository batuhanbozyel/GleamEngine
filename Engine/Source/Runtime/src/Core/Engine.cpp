#include "gpch.h"
#include "Engine.h"

#include "EventSystem.h"
#include "WindowSystem.h"
#include "ConfigSystem.h"
#include "IO/FileWatcher.h"
#include "Input/InputSystem.h"
#include "World/ScriptingSystem.h"
#include "Renderer/RenderSystem.h"

namespace Gleam {

void Engine::Initialize(const CommandLine& cli)
{
	int logLevel = cli("-log-level", static_cast<int>(Logger::Level::Info));
	Logger::SetLevel(static_cast<Logger::Level>(logLevel));
	
	// setup directories
	Globals::StartupDirectory = Filesystem::WorkingDirectory();
	Globals::BuiltinAssetsDirectory = Globals::StartupDirectory / "Assets";
	Globals::UserDataDirectory = Filesystem::AppDataDirectory() / "GleamEngine";
	Filesystem::CreateDirectories(Globals::UserDataDirectory);

	const auto projectFile = Path(cli.GetParam<TString>("project", TString()));
	if (projectFile.Empty() == false and Filesystem::Exists(projectFile))
	{
		AddSubsystem<ConfigSystem>(projectFile.Parent() / "ProjectSettings" / "Engine.config");
	}
	else
	{
		auto configSystem = AddSubsystem<ConfigSystem>(Globals::UserDataDirectory / "Launcher" / "Launcher.config");
		configSystem->Register(WindowConfig{ .windowFlag = WindowFlag::FixedWindow, .size = Size(1280.0f, 800.0f) });
	}

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

} // namespace Gleam

