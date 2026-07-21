//
//  MenuBar.cpp
//  Editor
//
//  Created by Batuhan Bozyel on 19.05.2023.
//

#include "MenuBar.h"

#include "Core/Globals.h"
#include "Core/Engine.h"
#include "Core/Application.h"
#include "Core/Events/Event.h"
#include "Core/Events/ApplicationEvent.h"

#include "Renderer/Renderers/ImGuiRenderer.h"
#include "World/WorldManager.h"
#include "World/World.h"

#include "View/ViewStack.h"
#include "View/Panels/Project/ProjectSettings.h"

#include <imgui.h>

using namespace GEditor;

void MenuBar::OnCreate(Gleam::World* world)
{
	mWorld = world;
}

void MenuBar::Render(Gleam::ImGuiRenderer* imgui)
{
	imgui->PushView([this](const Gleam::ImGuiPassData& passData)
	{
		if (!ImGui::BeginMenuBar()) { return; }
		
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("Save"))
			{
				auto worldManager = Gleam::Globals::GameInstance->GetSubsystem<Gleam::WorldManager>();
				worldManager->SaveActiveWorld();
			}

			if (ImGui::MenuItem("Exit"))
			{
				Gleam::EventDispatcher<Gleam::AppCloseEvent>::Publish(Gleam::AppCloseEvent());
			}

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Edit"))
		{
			if (ImGui::MenuItem("Project Settings"))
			{
				auto viewStack = mWorld->GetSubsystem<ViewStack>();
				auto projectSettings = viewStack->GetView<ProjectSettings>();
				projectSettings->Open();
			}

			ImGui::EndMenu();
		}

		ImGui::EndMenuBar();
	});
}
