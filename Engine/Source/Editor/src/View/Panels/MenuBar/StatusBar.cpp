#include "StatusBar.h"
#include "View/ViewStack.h"
#include "View/GleamTheme.h"
#include "View/Widgets/EditorWidgets.h"

#include "Core/Application.h"
#include "Renderer/Renderers/ImGuiRenderer.h"

#include <imgui_internal.h>

using namespace GEditor;

#if defined(USE_DIRECTX_RENDERER)
static constexpr const char* kBackendName = "D3D12";
#elif defined(USE_METAL_RENDERER)
static constexpr const char* kBackendName = "Metal";
#endif

void StatusBar::OnCreate(Gleam::Application* app)
{
	mFonts = &app->GetSubsystem<ViewStack>()->GetFonts();
}

void StatusBar::Render(Gleam::ImGuiRenderer* imgui)
{
	imgui->PushView([this](const Gleam::ImGuiPassData& passData)
	{
		constexpr float kHeight = 26.0f;
		constexpr float kPadding = 12.0f;

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		ImGui::PushStyleColor(ImGuiCol_WindowBg, Gleam::Theme::TitleBar);
		const bool visible = ImGui::BeginViewportSideBar("##StatusBar", ImGui::GetMainViewport(), ImGuiDir_Down, kHeight, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings);
		ImGui::PopStyleColor();
		ImGui::PopStyleVar(2);

		if (visible)
		{
			auto drawList = ImGui::GetWindowDrawList();
			const ImVec2 min = ImGui::GetWindowPos();
			const ImVec2 size = ImGui::GetWindowSize();
			drawList->AddLine(min, ImVec2(min.x + size.x, min.y), Widgets::ColorU32(Gleam::Theme::Divider));

			const float centerY = min.y + size.y * 0.5f;
			drawList->AddCircleFilled(ImVec2(min.x + kPadding + 3.0f, centerY), 3.0f, Widgets::ColorU32(Gleam::Theme::Success));

			ImGui::PushFont(mFonts->caption);
			drawList->AddText(ImVec2(min.x + kPadding + 12.0f, centerY - ImGui::GetFontSize() * 0.5f), Widgets::ColorU32(Gleam::Theme::TextMuted), "Ready");
			ImGui::PopFont();

			ImGui::PushFont(mFonts->mono);
			const ImVec2 backendSize = ImGui::CalcTextSize(kBackendName);
			drawList->AddText(ImVec2(min.x + size.x - kPadding - backendSize.x, centerY - ImGui::GetFontSize() * 0.5f), Widgets::ColorU32(Gleam::Theme::TextMuted), kBackendName);
			ImGui::PopFont();
		}
		ImGui::End();
	});
}
