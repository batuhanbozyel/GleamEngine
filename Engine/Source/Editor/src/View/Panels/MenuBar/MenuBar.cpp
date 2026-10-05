//
//  MenuBar.cpp
//  Editor
//
//  Created by Batuhan Bozyel on 19.05.2023.
//

#include "MenuBar.h"

#include "Core/Engine.h"
#include "Core/Globals.h"
#include "Core/Application.h"
#include "Core/PlatformTargetDefines.h"
#include "Core/Events/Event.h"
#include "Core/Events/ApplicationEvent.h"

#include "Renderer/Renderers/ImGuiRenderer.h"
#include "World/World.h"

#include "EWorld/EWorldManager.h"
#include "Undo/UndoSystem.h"
#include "View/ViewStack.h"
#include "View/EditorLayout.h"
#include "View/GleamTheme.h"
#include "View/IconsLucide.h"
#include "View/Panels/Project/ProjectSettings.h"
#include "View/Panels/Project/Preferences.h"

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

MenuBar::MenuBar(Gleam::World* world)
	: mWorld(world)
{

}

void MenuBar::OnCreate(Gleam::Application* app)
{
	mApplication = app;
	mFonts = &app->GetSubsystem<ViewStack>()->GetFonts();
}

void MenuBar::Render(Gleam::ImGuiRenderer* imgui)
{
	imgui->PushView([this, imgui](const Gleam::ImGuiPassData& passData)
	{
		auto undoSystem = mWorld->GetSubsystem<UndoSystem>();
		auto worldManager = mApplication->GetSubsystem<EWorldManager>();
		const bool simulating = worldManager->IsSimulating();

		// Routed globally so the shortcut reaches any panel, while an active text field keeps its own
		if (simulating == false and ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_Z, ImGuiInputFlags_RouteGlobal))
		{
			undoSystem->RequestUndo();
		}
		if (simulating == false and (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_Z, ImGuiInputFlags_RouteGlobal)
			|| ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_Y, ImGuiInputFlags_RouteGlobal)))
		{
			undoSystem->RequestRedo();
		}

		if (!ImGui::BeginMenuBar()) { return; }

		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 4.0f);
		ImGui::TextColored(Gleam::Theme::Accent, ICON_LC_SPARKLE);
		ImGui::SameLine(0.0f, 12.0f);
		ImGui::PushStyleColor(ImGuiCol_Text, Gleam::Theme::TextSecondary);

		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("Save", nullptr, false, simulating == false))
			{
				worldManager->Save();
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
			if (ImGui::MenuItem(undoLabel.c_str(), kUndoShortcut, false, simulating == false and undoSystem->CanUndo()))
			{
				undoSystem->RequestUndo();
			}

			auto redoLabel = HistoryLabel("Redo", undoSystem->GetRedoName());
			if (ImGui::MenuItem(redoLabel.c_str(), kRedoShortcut, false, simulating == false and undoSystem->CanRedo()))
			{
				undoSystem->RequestRedo();
			}

			ImGui::Separator();

			if (ImGui::MenuItem("Project Settings"))
			{
				auto viewStack = mApplication->GetSubsystem<ViewStack>();
				auto projectSettings = viewStack->GetView<ProjectSettings>();
				projectSettings->Open();
			}

			if (ImGui::MenuItem("Preferences"))
			{
				auto viewStack = mApplication->GetSubsystem<ViewStack>();
				viewStack->GetView<Preferences>()->Open();
			}

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Window"))
		{
			if (ImGui::MenuItem("Reset Layout"))
			{
				imgui->ApplyLayout(EditorLayout);
			}

			ImGui::EndMenu();
		}
		ImGui::PopStyleColor();

		ImGui::PushFont(mFonts->footnote);
		constexpr const char* kTitleSuffix = " - GleamEngine";
		const float titleWidth = ImGui::CalcTextSize(Gleam::Globals::ProjectName.c_str()).x + ImGui::CalcTextSize(kTitleSuffix).x;
		const float titleX = (ImGui::GetWindowWidth() - titleWidth) * 0.5f;
		if (titleX > ImGui::GetCursorPosX())
		{
			ImGui::SetCursorPosX(titleX);
			ImGui::TextColored(Gleam::Theme::TextMuted, "%s", Gleam::Globals::ProjectName.c_str());
			ImGui::SameLine(0.0f, 0.0f);
			ImGui::TextColored(Gleam::Theme::TextDim, "%s", kTitleSuffix);
		}
		ImGui::PopFont();

		ImGui::EndMenuBar();
	});
}
