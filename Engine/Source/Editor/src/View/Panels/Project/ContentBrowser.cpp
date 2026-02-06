//
//  ContentBrowser.cpp
//  Editor
//
//  Created by Batuhan Bozyel on 14.11.2023.
//

#include "ContentBrowser.h"
#include "EAssets/MeshSource.h"
#include "EAssets/EAssetManager.h"

#include "Gleam.h"

#include <imgui.h>

using namespace GEditor;

ContentBrowser::ContentBrowser(EAssetManager* assetManager)
	: mAssetManager(assetManager)
{

}

void ContentBrowser::Init(Gleam::World* world)
{
	mAssetDirectory = Gleam::Globals::ProjectContentDirectory;
    mCurrentDirectory = mAssetDirectory;
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

bool ContentBrowser::ImportAsset(const Gleam::Path& path)
{
	if (path.Extension() == L".gltf")
	{
		auto assetRegistry = AssetRegistry(path.Parent());
		auto meshSource = MeshSource(mAssetManager, &assetRegistry);
		auto settings = MeshSource::ImportSettings();
		if (meshSource.Import(path, settings))
		{
			mAssetManager->Import(mCurrentDirectory, meshSource);
			return true;
		}
	}
	return false;
}

void ContentBrowser::DrawDirectoryTreeView(const Gleam::Path& node)
{
    Gleam::TString filename = node.Filename();
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
		if (node.Extension() == Gleam::Asset::Extension())
		{
			auto guid = Gleam::Guid(node.Stem());
			const auto& asset = mAssetManager->GetAsset(guid);

			auto label = asset.name;
			if (asset.type == Gleam::Reflection::GetClass<Gleam::MeshDescriptor>().Guid())
			{
				label += " (Mesh)";
			}
			else if (asset.type == Gleam::Reflection::GetClass<Gleam::Texture2DDescriptor>().Guid())
			{
				label += " (Texture)";
			}
			else if (asset.type == Gleam::Reflection::GetClass<Gleam::MaterialDescriptor>().Guid())
			{
				label += " (Material)";
			}
			else if (asset.type == Gleam::Reflection::GetClass<Gleam::MaterialInstanceDescriptor>().Guid())
			{
				label += " (MaterialInstance)";
			}
			else
			{
				label += " (UNKNOWN)";
			}

			ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_SpanAvailWidth;
			if (ImGui::TreeNodeEx(label.c_str(), flags))
			{
				if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
				{
					ImGui::SetDragDropPayload("GLEAM_ASSET", &asset, sizeof(AssetItem));
					ImGui::Text("%s", label.c_str());
					ImGui::EndDragDropSource();
				}
			}
		}
		else if (node.Extension() == Gleam::Prefab::Extension())
		{
			auto guid = Gleam::Guid(node.Stem());
			const auto& asset = mAssetManager->GetAsset(guid);
			auto label = asset.name + " (Prefab)";

			ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_SpanAvailWidth;
			if (ImGui::TreeNodeEx(label.c_str(), flags))
			{
				if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
				{
					ImGui::SetDragDropPayload("GLEAM_PREFAB", &asset, sizeof(AssetItem));
					ImGui::Text("%s", label.c_str());
					ImGui::EndDragDropSource();
				}
			}
		}
		else if (node.Extension() == Gleam::World::Extension())
		{
			auto guid = Gleam::Guid(node.Stem());
			const auto& asset = mAssetManager->GetAsset(guid);
			auto label = asset.name + " (World)";

			ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_SpanAvailWidth;
			if (ImGui::TreeNodeEx(label.c_str(), flags))
			{
				if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
				{
					ImGui::SetDragDropPayload("GLEAM_WORLD", &asset, sizeof(AssetItem));
					ImGui::Text("%s", label.c_str());
					ImGui::EndDragDropSource();
				}
			}
		}
    }
    ImGui::PopID();
}
