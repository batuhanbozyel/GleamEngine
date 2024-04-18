//
//  ContentBrowser.cpp
//  Editor
//
//  Created by Batuhan Bozyel on 14.11.2023.
//

#include "ContentBrowser.h"
#include "Gleam.h"

#include "EAssets/MeshSource.h"

#include <imgui.h>

using namespace GEditor;

ContentBrowser::ContentBrowser()
	: mAssetDirectory(GameInstance->GetDefaultAssetPath())
{
    mCurrentDirectory = mAssetDirectory;
    mAssetManager = EAssetManager(mAssetDirectory);
}

void ContentBrowser::Render(Gleam::ImGuiRenderer* imgui)
{
	imgui->PushView([this](const Gleam::ImGuiPassData& passData)
	{
		ImGui::Begin("Content Browser");
        
		if (ImGui::Button("Import"))
		{
			auto& entityManager = Gleam::World::active->GetEntityManager();
			auto files = Gleam::FileDialog::Open();
			for (const auto& path : files)
			{
				ImportAsset(path);
			}
		}
        DrawDirectoryTreeView(mAssetDirectory);

		ImGui::End();
	});
}

bool ContentBrowser::ImportAsset(const Gleam::Filesystem::path& path)
{
	if (path.extension() == ".gltf")
	{
		auto meshSource = MeshSource();
		auto settings = MeshSource::ImportSettings();
		if (meshSource.Import(path, settings))
		{
			mAssetManager.Import(mCurrentDirectory, meshSource);
			return true;
		}
	}
	return false;
}

void ContentBrowser::DrawDirectoryTreeView(const Gleam::Filesystem::path& node)
{
    auto filename = node.filename().string();
    ImGui::PushID(filename.c_str());
    if (Gleam::Filesystem::is_directory(node))
    {
        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
        if (ImGui::TreeNodeEx(filename.c_str(), flags))
        {
            for (auto& entry : Gleam::Filesystem::directory_iterator(node))
            {
                DrawDirectoryTreeView(entry);
            }
            ImGui::TreePop();
        }
    }
    else
    {
        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_SpanAvailWidth;
        if (ImGui::TreeNodeEx(filename.c_str(), flags))
        {
			if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
			{
				const auto& asset = mAssetManager.GetAsset(node);
				ImGui::SetDragDropPayload("ASSET", &asset, sizeof(Gleam::Asset));
				ImGui::Text("%s", filename.c_str());
				ImGui::EndDragDropSource();
			}
        }
    }
    ImGui::PopID();
}
