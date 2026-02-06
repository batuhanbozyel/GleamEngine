#include "gpch.h"
#include "Engine.h"
#include "Globals.h"
#include "Application.h"

#include "EventSystem.h"
#include "Input/InputSystem.h"
#include "Renderer/RenderSystem.h"

#include "World/WorldManager.h"
#include "Assets/AssetManager.h"

using namespace Gleam;

Application::Application(const Project& project)
	: mProject(project)
{
	// setup globals
	Globals::GameInstance = this;
	Globals::ProjectName = project.name;
	Globals::ProjectDirectory = project.path;
	Globals::ProjectContentDirectory = Globals::ProjectDirectory/"Assets";

	// init game instance subsystems
	if (project.worldConfig.worlds.empty() == false)
	{
		auto assetManager = AddSubsystem<AssetManager>();
		auto worldManager = AddSubsystem<WorldManager>();
		worldManager->Configure(project.worldConfig);
		worldManager->OpenWorld(project.worldConfig.startingWorldIndex);
	}
	
	EventDispatcher<AppCloseEvent>::Subscribe([this](AppCloseEvent e)
	{
		mRunning = false;
	});
}

void Application::Run()
{
	auto eventSystem = Globals::Engine->GetSubsystem<EventSystem>();
    auto inputSystem = Globals::Engine->GetSubsystem<InputSystem>();
    auto renderSystem = Globals::Engine->GetSubsystem<RenderSystem>();
	auto worldManager = GetSubsystem<WorldManager>();

	while (mRunning)
	{
		EventDispatcher<AppTickEvent>::Publish(AppTickEvent());
        eventSystem->Update();
        inputSystem->Update();

		auto world = worldManager->GetActiveWorld();
        world->Update();

		renderSystem->PreRender(world);
        renderSystem->Render(world);
	}
}

Application::~Application()
{
    for (auto system : mSubsystems)
    {
        system->Shutdown(this);
    }
	mSubsystems.clear();
}
