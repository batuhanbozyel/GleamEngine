//
//  LaunchFailureDialog.cpp
//  Editor
//

#include "LaunchFailureDialog.h"
#include "View/ViewStack.h"
#include "View/GleamTheme.h"
#include "View/Widgets/EditorWidgets.h"

#include "Core/Application.h"
#include "Renderer/Renderers/ImGuiRenderer.h"

#include <imgui.h>

using namespace GEditor;

LaunchFailureDialog::LaunchFailureDialog(const LaunchFailure& failure)
	: mFailure(failure)
{

}

void LaunchFailureDialog::OnCreate(Gleam::Application* app)
{
	mFonts = &app->GetSubsystem<ViewStack>()->GetFonts();
}

void LaunchFailureDialog::Render(Gleam::ImGuiRenderer* imgui)
{
	imgui->PushView([this](const Gleam::ImGuiPassData& passData)
	{
		if (mOpenPopup)
		{
			mOpenPopup = false;
			ImGui::OpenPopup(mFailure.title.c_str());
		}

		const ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(viewport->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
		ImGui::SetNextWindowSize(ImVec2(viewport->Size.x * 0.7f, viewport->Size.y * 0.7f), ImGuiCond_Appearing);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(24.0f, 20.0f));
		const bool open = ImGui::BeginPopupModal(mFailure.title.c_str(), nullptr, ImGuiWindowFlags_NoSavedSettings);
		ImGui::PopStyleVar();
		if (open)
		{
			ImGui::PushFont(mFonts->caption);
			ImGui::TextColored(Gleam::Theme::TextDim, "LOG");
			ImGui::PopFont();

			constexpr float kButtonHeight = 36.0f;
			const float footerHeight = kButtonHeight + ImGui::GetStyle().ItemSpacing.y * 2.0f;
			ImGui::BeginChild("Log", ImVec2(0.0f, -footerHeight), ImGuiChildFlags_Borders, ImGuiWindowFlags_HorizontalScrollbar);
			ImGui::PushFont(mFonts->mono);
			ImGui::TextUnformatted(mFailure.log.data(), mFailure.log.data() + mFailure.log.size());
			ImGui::PopFont();
			// The failure is at the end of the log
			if (mScrollToBottom)
			{
				mScrollToBottom = false;
				ImGui::SetScrollHereY(1.0f);
			}
			ImGui::EndChild();

			ImGui::PushFont(mFonts->semiBold);
			if (Widgets::OutlineButton("Copy Log", ImVec2(120.0f, kButtonHeight)))
			{
				ImGui::SetClipboardText(mFailure.log.c_str());
			}
			ImGui::SameLine();
			if (Widgets::AccentButton("Close", ImVec2(120.0f, kButtonHeight)))
			{
				ImGui::CloseCurrentPopup();
			}
			ImGui::PopFont();
			ImGui::EndPopup();
		}
	});
}
