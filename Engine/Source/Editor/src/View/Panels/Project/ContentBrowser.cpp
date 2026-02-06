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

		ImGui::Separator();

		static float leftPanelWidth = 250.0f;

		ImGui::BeginChild("DirectoryTree", ImVec2(leftPanelWidth, 0), true);
		ImGui::Text("Directories");
		ImGui::Separator();
		DrawDirectoryTree(mAssetDirectory);
		ImGui::EndChild();

		ImGui::SameLine();

		ImGui::Button("##splitter", ImVec2(4.0f, -1));
		if (ImGui::IsItemActive())
		{
			leftPanelWidth += ImGui::GetIO().MouseDelta.x;
			leftPanelWidth = ImGui::GetIO().MousePos.x - ImGui::GetWindowPos().x;
		}

		if (ImGui::IsItemHovered())
		{
			ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
		}

		ImGui::SameLine();

		ImGui::BeginChild("AssetGrid", ImVec2(0, 0), true);
		ImGui::Text("Assets");
		ImGui::Separator();
		DrawAssetGrid();
		ImGui::EndChild();

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

void ContentBrowser::DrawDirectoryTree(const Gleam::Path& node)
{
	if (not Gleam::Filesystem::IsDirectory(node))
	{
		return;
	}

	Gleam::TString filename = node.Filename();
	ImGui::PushID(filename.c_str());

	ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow |
		ImGuiTreeNodeFlags_OpenOnDoubleClick |
		ImGuiTreeNodeFlags_SpanAvailWidth;

	if (node == mCurrentDirectory)
	{
		flags |= ImGuiTreeNodeFlags_Selected;
	}

	Gleam::Filesystem::ForEach(node, [&flags](const auto& entry)
	{
		if (Gleam::Filesystem::IsDirectory(entry))
		{
			flags |= ImGuiTreeNodeFlags_Leaf;
		}
	}, false);

	bool opened = ImGui::TreeNodeEx(filename.c_str(), flags);

	if (ImGui::IsItemClicked())
	{
		mCurrentDirectory = node;
	}

	if (opened)
	{
		Gleam::Filesystem::ForEach(node, [this](const auto& entry)
		{
			if (Gleam::Filesystem::IsDirectory(entry))
			{
				DrawDirectoryTree(entry);
			}
		}, false);
		ImGui::TreePop();
	}
	ImGui::PopID();
}

void ContentBrowser::DrawAssetGrid()
{
	static float iconSize = 80.0f;
	static float padding = 10.0f;

	float cellSize = iconSize + padding;
	float panelWidth = ImGui::GetContentRegionAvail().x;
	uint32_t columnCount = Gleam::Math::Max((uint32_t)(panelWidth / cellSize), 1u);

	uint32_t currentColumn = 0u;
	Gleam::Filesystem::ForEach(mCurrentDirectory, [&](const auto& entry)
	{
		if (Gleam::Filesystem::IsDirectory(entry))
		{
			return; // Skip directories in asset grid
		}

		AssetItem asset;
		Gleam::TString label;
		const char* iconText = "?";
		const char* payloadType = nullptr;
		ImVec4 assetColor = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);

		if (entry.Extension() == Gleam::Asset::Extension())
		{
			auto guid = Gleam::Guid(entry.Stem());
			asset = mAssetManager->GetAsset(guid);
			label = asset.name;

			if (asset.type == Gleam::Reflection::GetClass<Gleam::MeshDescriptor>().Guid())
			{
				iconText = "Mesh";
				payloadType = "GLEAM_ASSET";
				assetColor = ImVec4(0.2f, 0.5f, 0.95f, 1.0f); // Blue
			}
			else if (asset.type == Gleam::Reflection::GetClass<Gleam::Texture2DDescriptor>().Guid())
			{
				iconText = "Texture";
				payloadType = "GLEAM_ASSET";
				assetColor = ImVec4(0.95f, 0.3f, 0.7f, 1.0f); // Pink/Magenta
			}
			else if (asset.type == Gleam::Reflection::GetClass<Gleam::MaterialDescriptor>().Guid())
			{
				iconText = "Material";
				payloadType = "GLEAM_ASSET";
				assetColor = ImVec4(0.4f, 0.85f, 0.3f, 1.0f); // Green
			}
			else if (asset.type == Gleam::Reflection::GetClass<Gleam::MaterialInstanceDescriptor>().Guid())
			{
				iconText = "Material\nInstance";
				payloadType = "GLEAM_ASSET";
				assetColor = ImVec4(0.5f, 0.95f, 0.4f, 1.0f); // Light Green
			}
			else
			{
				iconText = "?";
				payloadType = "GLEAM_ASSET";
			}
		}
		else if (entry.Extension() == Gleam::Prefab::Extension())
		{
			auto guid = Gleam::Guid(entry.Stem());
			asset = mAssetManager->GetAsset(guid);
			label = asset.name;

			iconText = "Prefab";
			payloadType = "GLEAM_PREFAB";
			assetColor = ImVec4(0.9f, 0.55f, 0.2f, 1.0f); // Orange
		}
		else if (entry.Extension() == Gleam::World::Extension())
		{
			auto guid = Gleam::Guid(entry.Stem());
			asset = mAssetManager->GetAsset(guid);
			label = asset.name;

			iconText = "World";
			payloadType = "GLEAM_WORLD";
			assetColor = ImVec4(0.7f, 0.3f, 0.85f, 1.0f); // Purple
		}
		else
		{
			return; // Skip unknown file types
		}

		ImGui::PushID(label.c_str());

		ImGui::BeginGroup();

		ImVec4 hoverColor = ImVec4(assetColor.x * 1.3f, assetColor.y * 1.3f, assetColor.z * 1.3f, 1.0f);
		ImVec4 activeColor = ImVec4(assetColor.x * 1.5f, assetColor.y * 1.5f, assetColor.z * 1.5f, 1.0f);

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.4f, 0.4f, 0.4f, 1.0f));

		ImGui::Button(iconText, ImVec2(iconSize, iconSize));

		ImGui::PopStyleColor(3);

		if (payloadType && ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
		{
			ImGui::SetDragDropPayload(payloadType, &asset, sizeof(AssetItem));
			ImGui::Text("%s", label.c_str());
			ImGui::EndDragDropSource();
		}

		ImVec2 separatorStart = ImGui::GetCursorScreenPos();
		ImVec2 separatorEnd = ImVec2(separatorStart.x + iconSize, separatorStart.y);
		ImGui::GetWindowDrawList()->AddLine(separatorStart, separatorEnd, ImGui::ColorConvertFloat4ToU32(assetColor), 3.0f);
		ImGui::Spacing();

		ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + iconSize);
		ImGui::TextWrapped("%s", label.c_str());
		ImGui::PopTextWrapPos();

		ImGui::EndGroup();

		currentColumn++;
		if (currentColumn < columnCount)
		{
			ImGui::SameLine();
		}
		else
		{
			currentColumn = 0u;
		}

		ImGui::PopID();
	}, false);
}
