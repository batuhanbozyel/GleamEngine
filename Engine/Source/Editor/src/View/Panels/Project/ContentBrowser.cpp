//
//  ContentBrowser.cpp
//  Editor
//
//  Created by Batuhan Bozyel on 14.11.2023.
//

#include "ContentBrowser.h"
#include "EAssets/MeshSource.h"
#include "EAssets/EAssetManager.h"
#include "View/Widgets/AssetIcon.h"
#include "View/Widgets/ThumbnailGrid.h"

#include "Core/Globals.h"
#include "Core/Engine.h"

#include "Renderer/Renderers/ImGuiRenderer.h"

#include "World/World.h"
#include "World/Prefab.h"

#include "IO/FileDialog.h"

#include <imgui.h>

using namespace GEditor;

ContentBrowser::ContentBrowser(EAssetManager* assetManager)
	: mAssetManager(assetManager)
{

}

void ContentBrowser::OnCreate(Gleam::Application* app)
{
	mAssetDirectory = Gleam::Globals::ProjectContentDirectory;

	SetCurrentDir(mAssetDirectory);
	RefreshDirectoryTree();

	auto fileWatcher = Gleam::Globals::Engine->GetSubsystem<Gleam::FileWatcher>();
	mWatchHandle = fileWatcher->AddWatch(mAssetDirectory, [this](const Gleam::Path& path, Gleam::FileWatchEvent event)
	{
		mRefreshRequested.store(true, std::memory_order_relaxed);
	});
}

void ContentBrowser::OnDestroy(Gleam::Application* app)
{
	if (mWatchHandle)
	{
		auto fileWatcher = Gleam::Globals::Engine->GetSubsystem<Gleam::FileWatcher>();
		fileWatcher->RemoveWatch(mWatchHandle);
		mWatchHandle = nullptr;
	}
}

void ContentBrowser::Render(Gleam::ImGuiRenderer* imgui)
{
	imgui->PushView([this](const Gleam::ImGuiPassData& passData)
	{
		ImGui::Begin("Content Browser");

		if (mRefreshRequested.exchange(false, std::memory_order_relaxed))
		{
			RefreshAssetGrid();
			RefreshDirectoryTree();
		}

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

		ImGui::BeginChild("DirectoryTree", ImVec2(leftPanelWidth, 0), ImGuiChildFlags_Borders);
		ImGui::Text("Directories");
		ImGui::Separator();
		DrawDirectoryTree();
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

		ImGui::BeginChild("AssetGrid", ImVec2(0, 0), ImGuiChildFlags_Borders);

		if (ImGui::Button(Gleam::TStringView(mAssetDirectory.Stem()).data()))
		{
			SetCurrentDir(mAssetDirectory);
		}

		if (mCurrentDirectory != mAssetDirectory)
		{
			uint32_t directoryID = 0;
			Gleam::Path breadcrumbPath = mAssetDirectory;
			auto relativePath = Gleam::Filesystem::Relative(mCurrentDirectory, mAssetDirectory);
			for (const auto& directory : relativePath.Split())
			{
				ImGui::SameLine();
				ImGui::Text("/");
				ImGui::SameLine();

				breadcrumbPath = breadcrumbPath / directory;

				ImGui::PushID(directoryID);
				if (ImGui::Button(Gleam::TStringView(directory).data()))
				{
					SetCurrentDir(breadcrumbPath);
				}
				ImGui::PopID();
			}
		}

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

void ContentBrowser::SetCurrentDir(const Gleam::Path& directory)
{
	mCurrentDirectory = directory;
	RefreshAssetGrid();
}

void ContentBrowser::RefreshAssetGrid()
{
	mGridEntries.clear();
	Gleam::Filesystem::ForEach(mCurrentDirectory, [&](const auto& node)
	{
		GridEntry entry;
		entry.path = node;
		entry.isDirectory = node.IsDirectory();
		entry.color = Gleam::Color(0.5f, 0.5f, 0.5f, 1.0f);

		if (entry.isDirectory)
		{
			entry.label = node.Filename();
			entry.color = Gleam::Color(0.9f, 0.75f, 0.3f, 1.0f); // Yellow/Gold
		}
		else if (node.Extension() == Gleam::Asset::Extension())
		{
			entry.asset = mAssetManager->GetAsset(Gleam::Guid(node.Stem()));
			entry.label = entry.asset.name;
			entry.payloadType = "GLEAM_ASSET";

			const auto icon = GetAssetIcon(entry.asset.type);
			entry.iconText = icon.text;
			entry.color = icon.color;
		}
		else if (node.Extension() == Gleam::Prefab::Extension())
		{
			entry.asset = mAssetManager->GetAsset(Gleam::Guid(node.Stem()));
			entry.label = entry.asset.name;
			entry.payloadType = "GLEAM_PREFAB";

			const auto icon = GetAssetIcon(entry.asset.type);
			entry.iconText = icon.text;
			entry.color = icon.color;
		}
		else if (node.Extension() == Gleam::World::Extension())
		{
			entry.asset = mAssetManager->GetAsset(Gleam::Guid(node.Stem()));
			entry.label = entry.asset.name;
			entry.payloadType = "GLEAM_WORLD";

			const auto icon = GetAssetIcon(entry.asset.type);
			entry.iconText = icon.text;
			entry.color = icon.color;
		}
		else
		{
			return; // Skip unknown file types
		}

		mGridEntries.push_back(eastl::move(entry));
	}, false);

	std::sort(mGridEntries.begin(), mGridEntries.end(), [](const GridEntry& lhs, const GridEntry& rhs)
	{
		if (lhs.isDirectory != rhs.isDirectory)
		{
			return lhs.isDirectory;
		}
		return lhs.label < rhs.label;
	});
}

void ContentBrowser::RefreshDirectoryTree()
{
	mTreeEntries.clear();
	BuildDirectoryTree(mAssetDirectory);
}

void ContentBrowser::BuildDirectoryTree(const Gleam::Path& node)
{
	uint32_t self = static_cast<uint32_t>(mTreeEntries.size());
	mTreeEntries.push_back(TreeEntry{ .path = node, .label = node.Filename() });

	Gleam::TArray<Gleam::Path> subdirectories;
	Gleam::Filesystem::ForEach(node, [&subdirectories](const auto& entry)
	{
		if (entry.IsDirectory())
		{
			subdirectories.push_back(entry);
		}
	}, false);

	std::sort(subdirectories.begin(), subdirectories.end());

	for (const auto& subdirectory : subdirectories)
	{
		BuildDirectoryTree(subdirectory);
	}

	mTreeEntries[self].descendantCount = static_cast<uint32_t>(mTreeEntries.size()) - self - 1u;
}

void ContentBrowser::DrawDirectoryTree()
{
	uint32_t index = 0u;
	while (index < mTreeEntries.size())
	{
		index = DrawDirectoryNode(index);
	}
}

uint32_t ContentBrowser::DrawDirectoryNode(uint32_t index)
{
	const auto& entry = mTreeEntries[index];
	uint32_t next = index + 1u + entry.descendantCount;

	ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow |
		ImGuiTreeNodeFlags_OpenOnDoubleClick |
		ImGuiTreeNodeFlags_SpanAvailWidth;

	if (entry.descendantCount == 0u)
	{
		flags |= ImGuiTreeNodeFlags_Leaf;
	}

	if (entry.path == mCurrentDirectory)
	{
		flags |= ImGuiTreeNodeFlags_Selected;
	}

	ImGui::PushID(static_cast<int>(index));
	bool opened = ImGui::TreeNodeEx(entry.label.c_str(), flags);

	if (ImGui::IsItemClicked())
	{
		SetCurrentDir(entry.path);
	}

	if (opened)
	{
		uint32_t child = index + 1u;
		while (child < next)
		{
			child = DrawDirectoryNode(child);
		}
		ImGui::TreePop();
	}
	ImGui::PopID();

	return next;
}

void ContentBrowser::DrawAssetGrid()
{
	Gleam::TArray<ThumbnailItem> items;
	items.reserve(mGridEntries.size());
	for (const auto& entry : mGridEntries)
	{
		items.push_back({
			.label = entry.label,
			.iconText = entry.iconText,
			.color = entry.color,
			.filled = entry.isDirectory,
			.payloadType = entry.payloadType,
			.payload = &entry.asset,
			.payloadSize = sizeof(AssetItem)
		});
	}

	const auto events = ThumbnailGrid::Draw(items);
	if (events.doubleClicked >= 0 and mGridEntries[events.doubleClicked].isDirectory)
	{
		SetCurrentDir(mGridEntries[events.doubleClicked].path);
	}
}
