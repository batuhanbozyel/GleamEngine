//
//  ProjectSettings.cpp
//  Editor
//

#include "ProjectSettings.h"
#include "View/Widgets/PropertyDrawer.h"

#include "Core/Globals.h"
#include "Core/Engine.h"
#include "Core/ConfigSystem.h"

#include "Renderer/Renderers/ImGuiRenderer.h"

#include <imgui.h>
#include <cstring>

using namespace GEditor;

void ProjectSettings::Render(Gleam::ImGuiRenderer* imgui)
{
	if (mIsOpen)
	{
		imgui->PushView([this](const Gleam::ImGuiPassData& passData)
		{
			ImGui::SetNextWindowSize(ImVec2(720.0f, 480.0f), ImGuiCond_FirstUseEver);
			if (ImGui::Begin("Project Settings", &mIsOpen))
			{
				constexpr float kCategoryPanelWidth = 200.0f;
				ImGui::BeginChild("##CategoryList", ImVec2(kCategoryPanelWidth, 0.0f), ImGuiChildFlags_Borders);
				DrawCategoryList();
				ImGui::EndChild();

				ImGui::SameLine();

				ImGui::BeginChild("##SettingsContent", ImVec2(0.0f, 0.0f), ImGuiChildFlags_None);
				DrawSettingsContent();
				ImGui::EndChild();
			}
			ImGui::End();
		});
	}
}

void ProjectSettings::DrawCategoryList()
{
	auto configSystem = Gleam::Globals::Engine->GetSubsystem<Gleam::ConfigSystem>();
	configSystem->ForEachConfig([this](const Gleam::ConfigSystem::ConfigView& config)
	{
		uint32_t typeHash = config.desc.TypeHash();
		if (mSelectedConfig == 0)
		{
			mSelectedConfig = typeHash;
		}

		auto name = config.desc.ResolveName();
		char label[128];
		std::memcpy(label, name.data(), name.size());
		label[name.size()] = '\0';

		if (ImGui::Selectable(label, mSelectedConfig == typeHash))
		{
			mSelectedConfig = typeHash;
		}
	});
}

void ProjectSettings::DrawSettingsContent()
{
	auto configSystem = Gleam::Globals::Engine->GetSubsystem<Gleam::ConfigSystem>();
	configSystem->ForEachConfig([this, configSystem](const Gleam::ConfigSystem::ConfigView& config)
	{
		if (mSelectedConfig == config.desc.TypeHash())
		{
			auto name = config.desc.ResolveName();
			char title[128];
			std::memcpy(title, name.data(), name.size());
			title[name.size()] = '\0';

			ImGui::Text("%s", title);
			ImGui::Separator();

			size_t size = config.desc.GetSize();
			Gleam::TArray<uint8_t> snapshot(size);
			std::memcpy(snapshot.data(), config.data, size);

			float panelWidth = ImGui::GetContentRegionAvail().x;
			PropertyDrawer::DrawClassFields(config.data, config.desc, panelWidth * 0.3f);

			if (std::memcmp(snapshot.data(), config.data, size) != 0)
			{
				configSystem->MarkModified(config.desc.TypeHash());
			}
		}
	});
}
