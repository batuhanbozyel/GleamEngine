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
#include "Core/PlatformTargetDefines.h"
#include "Core/Events/Event.h"
#include "Core/Events/ApplicationEvent.h"

#include "Renderer/Renderers/ImGuiRenderer.h"
#include "World/WorldManager.h"
#include "World/World.h"

#include "Undo/UndoSystem.h"
#include "View/ViewStack.h"
#include "View/Panels/Project/ProjectSettings.h"

#include <imgui.h>

using namespace GEditor;

namespace {

#ifdef PLATFORM_MACOS
constexpr const char* kUndoShortcut = "Cmd+Z";
constexpr const char* kRedoShortcut = "Cmd+Shift+Z";
#else
constexpr const char* kUndoShortcut = "Ctrl+Z";
constexpr const char* kRedoShortcut = "Ctrl+Shift+Z";
#endif

Gleam::TString HistoryLabel(const char* action, const Gleam::TStringView name)
{
	Gleam::TString label = action;
	if (name.empty() == false)
	{
		label.append(" ").append(name.data(), name.size());
	}
	return label;
}

} // namespace

void MenuBar::OnCreate(Gleam::World* world)
{
	mWorld = world;
}

void MenuBar::Render(Gleam::ImGuiRenderer* imgui)
{
	imgui->PushView([this](const Gleam::ImGuiPassData& passData)
	{
		auto undoSystem = mWorld->GetSubsystem<UndoSystem>();

		// Routed globally so the shortcut reaches any panel, while an active text field keeps its own
		if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_Z, ImGuiInputFlags_RouteGlobal))
		{
			undoSystem->RequestUndo();
		}
		if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_Z, ImGuiInputFlags_RouteGlobal)
			|| ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_Y, ImGuiInputFlags_RouteGlobal))
		{
			undoSystem->RequestRedo();
		}

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
			auto undoLabel = HistoryLabel("Undo", undoSystem->GetUndoName());
			if (ImGui::MenuItem(undoLabel.c_str(), kUndoShortcut, false, undoSystem->CanUndo()))
			{
				undoSystem->RequestUndo();
			}

			auto redoLabel = HistoryLabel("Redo", undoSystem->GetRedoName());
			if (ImGui::MenuItem(redoLabel.c_str(), kRedoShortcut, false, undoSystem->CanRedo()))
			{
				undoSystem->RequestRedo();
			}

			ImGui::Separator();

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
