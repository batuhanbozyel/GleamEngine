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

void ContentBrowser::Init(Gleam::World* world)
{
	mAssetDirectory = Gleam::Globals::ProjectContentDirectory;
    mCurrentDirectory = mAssetDirectory;
    mAssetRegistry = world->AddSubsystem<AssetRegistry>(mAssetDirectory);
}

void ContentBrowser::Render(Gleam::ImGuiRenderer* imgui)
{
	imgui->PushView([this](const Gleam::ImGuiPassData& passData)
	{
		ImGui::Begin("Content Browser");
        
		if (ImGui::Button("Import"))
		{
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

bool ContentBrowser::ImportAsset(const Gleam::Filesystem::Path& path)
{
	if (path.extension() == ".gltf")
	{
		auto meshSource = MeshSource(mAssetRegistry);
		auto settings = MeshSource::ImportSettings();
		if (meshSource.Import(path, settings))
		{
			mAssetRegistry->Import(mCurrentDirectory, meshSource);
			return true;
		}
	}
	return false;
}

void ContentBrowser::DrawDirectoryTreeView(const Gleam::Filesystem::Path& node)
{
    auto filename = node.filename().string();
    ImGui::PushID(filename.c_str());
    if (Gleam::Filesystem::IsDirectory(node))
    {
        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
        if (ImGui::TreeNodeEx(filename.c_str(), flags))
        {
            Gleam::Filesystem::ForEach(node, [this](const auto& entry)
            {
                DrawDirectoryTreeView(entry);
            }, false);
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
				auto guid = Gleam::Guid(node.stem().string());
				const auto& asset = mAssetRegistry->GetAsset(guid);
				ImGui::SetDragDropPayload("EDITOR_ASSET", &asset, sizeof(AssetItem));
				ImGui::Text("%s", filename.c_str());
				ImGui::EndDragDropSource();
			}
        }
    }
    ImGui::PopID();
}
