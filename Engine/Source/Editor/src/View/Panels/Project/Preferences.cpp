#include "Preferences.h"
#include "Config/EditorConfigSystem.h"
#include "Config/EditorPreferences.h"
#include "View/ViewStack.h"
#include "View/GleamTheme.h"

#include "Core/Globals.h"
#include "Core/Engine.h"
#include "Core/Application.h"

#include "Renderer/Renderers/ImGuiRenderer.h"

#include <Editor.Reflection.generated.h>

#include <imgui.h>

#include <cstdio>

using namespace GEditor;

static constexpr float kScaleOptions[] = { 0.75f, 0.875f, 1.0f, 1.125f, 1.25f, 1.5f, 1.75f, 2.0f };

void Preferences::OnCreate(Gleam::Application* app)
{
	mViewStack = app->GetSubsystem<ViewStack>();
}

void Preferences::Render(Gleam::ImGuiRenderer* imgui)
{
	if (mIsOpen)
	{
		imgui->PushView([this](const Gleam::ImGuiPassData& passData)
		{
			ImGui::SetNextWindowSize(ImVec2(420.0f, 160.0f), ImGuiCond_FirstUseEver);
			if (ImGui::Begin("Preferences", &mIsOpen, ImGuiWindowFlags_NoDocking))
			{
				auto editorConfig = Gleam::Globals::Engine->GetSubsystem<EditorConfigSystem>();
				auto preferences = editorConfig->Get<Gleam::EditorPreferences>();

				char preview[16];
				std::snprintf(preview, sizeof(preview), "%.1f%%", preferences.uiScale * 100.0f);

				ImGui::AlignTextToFramePadding();
				ImGui::TextColored(Gleam::Theme::TextSecondary, "Interface scale");
				ImGui::SameLine(140.0f);
				ImGui::SetNextItemWidth(-FLT_MIN);
				if (ImGui::BeginCombo("##InterfaceScale", preview))
				{
					for (float scale : kScaleOptions)
					{
						char label[16];
						std::snprintf(label, sizeof(label), "%.1f%%", scale * 100.0f);
						const bool selected = scale == preferences.uiScale;
						if (ImGui::Selectable(label, selected) and selected == false)
						{
							preferences.uiScale = scale;
							editorConfig->Set(preferences);
							mViewStack->SetFontScale(scale);
						}
					}
					ImGui::EndCombo();
				}
			}
			ImGui::End();
		});
	}
}
