#include "gpch.h"
#include "Engine.h"
#include "Globals.h"
#include "Application.h"

#include "EventSystem.h"
#include "Input/InputSystem.h"
#include "World/ScriptingSystem.h"
#include "Renderer/RenderSystem.h"

#include "World/WorldManager.h"
#include "Assets/AssetManager.h"

#include "Renderer/Renderers/WorldRenderer.h"
#include "Renderer/Renderers/PostProcessStack.h"
#include "Renderer/Material/MaterialSystem.h"

using namespace Gleam;

Application::Application(const Project& project)
	: mProject(project)
{
	// setup globals
	Globals::ProjectName = project.name;
	Globals::ProjectDirectory = Globals::StartupDirectory/project.path;
	Globals::BuiltinAssetsDirectory = Globals::StartupDirectory/"Assets";
	Globals::ProjectContentDirectory = Globals::ProjectDirectory/"Assets";
	
	// init game instance subsystems
    auto assetManager = AddSubsystem<AssetManager>();
	auto scriptingSystem = AddSubsystem<ScriptingSystem>();
	auto materialSystem = AddSubsystem<MaterialSystem>();
	auto worldManager = AddSubsystem<WorldManager>();
	worldManager->Configure(project.worldConfig);
    
    // add default renderers
    auto renderSystem = Globals::Engine->GetSubsystem<RenderSystem>();
    renderSystem->AddRenderer<WorldRenderer>();
    renderSystem->AddRenderer<PostProcessStack>();
	
	EventDispatcher<AppCloseEvent>::Subscribe([this](AppCloseEvent e)
	{
		mRunning = false;
	});

	worldManager->OpenWorld(project.worldConfig.startingWorldIndex);
}

void Application::Run()
{
	auto eventSystem = Globals::Engine->GetSubsystem<EventSystem>();
    auto inputSystem = Globals::Engine->GetSubsystem<InputSystem>();
    auto renderSystem = Globals::Engine->GetSubsystem<RenderSystem>();
	auto worldManager = GetSubsystem<WorldManager>();

	while (mRunning)
	{
        eventSystem->Update();
        inputSystem->Update();

		auto world = worldManager->GetActiveWorld();
        world->Update();

        renderSystem->Render(world);
	}
}

Application::~Application()
{
    for (auto system : mSubsystems)
    {
        system->Shutdown();
    }
	mSubsystems.clear();
}
