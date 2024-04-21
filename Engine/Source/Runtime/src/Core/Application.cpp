#include "gpch.h"
#include "Engine.h"
#include "Globals.h"
#include "Application.h"

#include "EventSystem.h"
#include "Input/InputSystem.h"
#include "Renderer/RenderSystem.h"

#include "World/WorldManager.h"
#include "Renderer/Renderers/WorldRenderer.h"
#include "Renderer/Renderers/PostProcessStack.h"
#include "Renderer/Material/MaterialSystem.h"

using namespace Gleam;

Application::Application(const Project& project)
	: mProject(project)
{
	// setup globals
	Globals::ProjectName = project.name;
	Globals::ProjectDirectory = project.path;
	Globals::BuiltinAssetsDirectory = Globals::StartupDirectory/"Assets";
	Globals::ProjectContentDirectory = Globals::ProjectDirectory/"Assets";
	
	// init world manager
	auto worldManager = AddSubsystem<WorldManager>();
	worldManager->Configure(project.worldConfig);
	AddSubsystem<MaterialSystem>();
    
    // add default renderers
    auto renderSystem = Globals::Engine->GetSubsystem<RenderSystem>();
    renderSystem->AddRenderer<WorldRenderer>();
    renderSystem->AddRenderer<PostProcessStack>();
	
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
        eventSystem->Update();
        inputSystem->Update();

		for (auto system : mTickableSubsystems)
		{
			system->Tick();
		}
        
		auto world = worldManager->GetActiveWorld();
        world->Update();

        renderSystem->Render();
	}
}

Application::~Application()
{
    for (auto system : mSubsystems)
    {
        system->Shutdown();
    }
	mSubsystems.clear();
	mTickableSubsystems.clear();
}
