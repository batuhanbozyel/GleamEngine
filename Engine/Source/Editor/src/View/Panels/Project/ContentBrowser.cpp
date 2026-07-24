//
//  ContentBrowser.cpp
//  Editor
//
//  Created by Batuhan Bozyel on 14.11.2023.
//

#include "ContentBrowser.h"
#include "EAssets/MeshSource.h"
#include "EAssets/EAssetManager.h"

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

void ContentBrowser::OnCreate(Gleam::World* world)
{
	mAssetDirectory = Gleam::Globals::ProjectContentDirectory;

	SetCurrentDirectory(mAssetDirectory);
	RefreshDirectoryTree();

	auto fileWatcher = Gleam::Globals::Engine->GetSubsystem<Gleam::FileWatcher>();
	mWatchHandle = fileWatcher->AddWatch(mAssetDirectory, [this](const Gleam::Path& path, Gleam::FileWatchEvent event)
	{
		mRefreshRequested.store(true, std::memory_order_relaxed);
	});
}

void ContentBrowser::OnDestroy(Gleam::World* world)
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
			SetCurrentDirectory(mAssetDirectory);
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
					SetCurrentDirectory(breadcrumbPath);
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

void ContentBrowser::SetCurrentDirectory(const Gleam::Path& directory)
{
	mCurrentDirectory = directory;
	RefreshAssetGrid();
}

void ContentBrowser::RefreshAssetGrid()
{
	static const auto meshType = Gleam::Reflection::GetClass<Gleam::MeshDescriptor>().Guid();
	static const auto textureType = Gleam::Reflection::GetClass<Gleam::Texture2DDescriptor>().Guid();
	static const auto materialType = Gleam::Reflection::GetClass<Gleam::MaterialDescriptor>().Guid();
	static const auto materialInstanceType = Gleam::Reflection::GetClass<Gleam::MaterialInstanceDescriptor>().Guid();

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

			if (entry.asset.type == meshType)
			{
				entry.iconText = "Mesh";
				entry.color = Gleam::Color(0.2f, 0.5f, 0.95f, 1.0f); // Blue
			}
			else if (entry.asset.type == textureType)
			{
				entry.iconText = "Texture";
				entry.color = Gleam::Color(0.95f, 0.3f, 0.7f, 1.0f); // Pink/Magenta
			}
			else if (entry.asset.type == materialType)
			{
				entry.iconText = "Material";
				entry.color = Gleam::Color(0.15f, 0.65f, 0.1f, 1.0f); // Green
			}
			else if (entry.asset.type == materialInstanceType)
			{
				entry.iconText = "Material\nInstance";
				entry.color = Gleam::Color(0.5f, 0.95f, 0.4f, 1.0f); // Light Green
			}
			else
			{
				entry.iconText = "?";
			}
		}
		else if (node.Extension() == Gleam::Prefab::Extension())
		{
			entry.asset = mAssetManager->GetAsset(Gleam::Guid(node.Stem()));
			entry.label = entry.asset.name;
			entry.iconText = "Prefab";
			entry.payloadType = "GLEAM_PREFAB";
			entry.color = Gleam::Color(0.9f, 0.55f, 0.2f, 1.0f); // Orange
		}
		else if (node.Extension() == Gleam::World::Extension())
		{
			entry.asset = mAssetManager->GetAsset(Gleam::Guid(node.Stem()));
			entry.label = entry.asset.name;
			entry.iconText = "World";
			entry.payloadType = "GLEAM_WORLD";
			entry.color = Gleam::Color(0.7f, 0.3f, 0.85f, 1.0f); // Purple
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
		SetCurrentDirectory(entry.path);
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
	static float iconSize = 80.0f;
	static float padding = 10.0f;

	float cellSize = iconSize + padding;
	float panelWidth = ImGui::GetContentRegionAvail().x;
	uint32_t columnCount = Gleam::Math::Max((uint32_t)(panelWidth / cellSize), 1u);
	uint32_t entryCount = static_cast<uint32_t>(mGridEntries.size());
	uint32_t rowCount = (entryCount + columnCount - 1u) / columnCount;

	float labelHeight = ImGui::GetTextLineHeight() * 2.0f;
	float rowHeight = iconSize + labelHeight + ImGui::GetStyle().ItemSpacing.y * 2.0f;

	ImGuiListClipper clipper;
	clipper.Begin(static_cast<int>(rowCount), rowHeight);
	while (clipper.Step())
	{
		for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; ++row)
		{
			for (uint32_t column = 0u; column < columnCount; ++column)
			{
				uint32_t index = static_cast<uint32_t>(row) * columnCount + column;
				if (index >= entryCount)
				{
					break;
				}

				if (column > 0u)
				{
					ImGui::SameLine();
				}
				DrawAssetItem(mGridEntries[index], index, iconSize);
			}
		}
	}
	clipper.End();

	if (not mPendingDirectory.Empty())
	{
		SetCurrentDirectory(mPendingDirectory);
		mPendingDirectory.Clear();
	}
}

void ContentBrowser::DrawAssetItem(const GridEntry& entry, uint32_t index, float iconSize)
{
	ImVec4 assetColor = ImVec4(entry.color.r, entry.color.g, entry.color.b, entry.color.a);

	ImGui::PushID(static_cast<int>(index));

	ImGui::BeginGroup();

	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.4f, 0.4f, 0.4f, 1.0f));

	if (entry.isDirectory)
	{
		ImVec4 hoverColor = ImVec4(assetColor.x * 1.1f, assetColor.y * 1.1f, assetColor.z * 1.1f, 1.0f);
		ImVec4 activeColor = ImVec4(assetColor.x * 1.3f, assetColor.y * 1.3f, assetColor.z * 1.3f, 1.0f);

		ImVec2 cursorPos = ImGui::GetCursorScreenPos();
		ImGui::InvisibleButton("##folder", ImVec2(iconSize, iconSize));

		ImVec4 currentColor = assetColor;
		if (ImGui::IsItemActive())
		{
			currentColor = activeColor;
		}
		else if (ImGui::IsItemHovered())
		{
			currentColor = hoverColor;
		}

		ImGui::GetWindowDrawList()->AddRectFilled(
			cursorPos,
			ImVec2(cursorPos.x + iconSize, cursorPos.y + iconSize),
			ImGui::ColorConvertFloat4ToU32(currentColor)
		);
	}
	else
	{
		ImGui::Button(entry.iconText, ImVec2(iconSize, iconSize));
	}

	ImGui::PopStyleColor(3);

	bool openDirectory = entry.isDirectory && ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left);

	if (entry.payloadType && ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
	{
		ImGui::SetDragDropPayload(entry.payloadType, &entry.asset, sizeof(AssetItem));
		ImGui::Text("%s", entry.label.c_str());
		ImGui::EndDragDropSource();
	}

	if (not entry.isDirectory)
	{
		ImVec2 separatorStart = ImGui::GetCursorScreenPos();
		ImVec2 separatorEnd = ImVec2(separatorStart.x + iconSize, separatorStart.y);
		ImGui::GetWindowDrawList()->AddLine(separatorStart, separatorEnd, ImGui::ColorConvertFloat4ToU32(assetColor), 3.0f);
	}
	ImGui::Spacing();

	// Clip the label to a fixed two-line box so every cell is the same height,
	// keeping the list clipper's row estimate exact
	float labelHeight = ImGui::GetTextLineHeight() * 2.0f;
	ImVec2 labelPos = ImGui::GetCursorScreenPos();

	ImGui::PushClipRect(labelPos, ImVec2(labelPos.x + iconSize, labelPos.y + labelHeight), true);
	ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + iconSize);
	ImGui::TextWrapped("%s", entry.label.c_str());
	ImGui::PopTextWrapPos();
	ImGui::PopClipRect();

	ImGui::SetCursorScreenPos(labelPos);
	ImGui::Dummy(ImVec2(iconSize, labelHeight));

	ImGui::EndGroup();

	ImGui::PopID();

	if (openDirectory)
	{
		mPendingDirectory = entry.path;
	}
}
