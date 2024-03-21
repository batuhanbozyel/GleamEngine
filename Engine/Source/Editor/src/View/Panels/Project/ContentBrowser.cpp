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
    mAssetManager = AssetManager(mAssetDirectory);
}

void ContentBrowser::Render(Gleam::ImGuiRenderer* imgui)
{
	imgui->PushView([this]()
	{
		ImGui::Begin("Content Browser");
        
		if (ImGui::Button("Import"))
		{
			auto& entityManager = Gleam::World::active->GetEntityManager();
			auto files = Gleam::FileDialog::Open();
			for (const auto& path : files)
			{
				if (path.extension() == ".gltf")
				{
                    auto meshSource = MeshSource();
                    auto settings = MeshSource::ImportSettings();
					bool success = meshSource.Import(path, settings);
                    mAssetManager.Import(mCurrentDirectory, meshSource);
				}
			}
		}
        ShowDirectoryContents(mAssetDirectory);

		ImGui::End();
	});
}

void ContentBrowser::ShowDirectoryContents(const Gleam::Filesystem::path& node)
{
    auto filename = node.filename();
    ImGui::PushID(filename.c_str());
    if (Gleam::Filesystem::is_directory(node))
    {
        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
        if (ImGui::TreeNodeEx(filename.c_str(), flags))
        {
            for (auto& entry : Gleam::Filesystem::directory_iterator(node))
            {
                ShowDirectoryContents(entry);
            }
            ImGui::TreePop();
        }
    }
    else
    {
        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_SpanAvailWidth;
        if (ImGui::TreeNodeEx(filename.c_str(), flags))
        {
            
        }
    }
    ImGui::PopID();
}
