//
//  ContentBrowser.cpp
//  Editor
//
//  Created by Batuhan Bozyel on 14.11.2023.
//

#include "ContentBrowser.h"
#include "EAssets/EAssetManager.h"
#include "View/Widgets/AssetIcon.h"
#include "View/Widgets/ThumbnailGrid.h"
#include "View/Widgets/EditorWidgets.h"
#include "View/Panels/Project/AssetImportDialog.h"
#include "View/ViewStack.h"
#include "View/EditorFonts.h"
#include "View/IconsLucide.h"

#include "Core/Globals.h"
#include "Core/Engine.h"
#include "Core/Application.h"

#include "Renderer/Renderers/ImGuiRenderer.h"

#include "World/World.h"
#include "World/Prefab.h"

#include "IO/FileDialog.h"

#include <imgui_internal.h>

using namespace GEditor;

static constexpr float kToolbarHeight = 44.0f;
static constexpr float kTreeWidth = 180.0f;
static constexpr float kTreeRowHeight = 26.0f;

struct CategoryChip
{
	const char* label;
	ImVec4 color;
	AssetCategory category;
};

static constexpr CategoryChip kCategoryChips[] = {
	{ "Prefab", Gleam::Theme::Warning, AssetCategory::Prefab },
	{ "Mesh", Gleam::Theme::Info, AssetCategory::Mesh },
	{ "Material", Gleam::Theme::Success, AssetCategory::Material },
	{ "Material Instance", Gleam::Theme::Teal, AssetCategory::MaterialInstance },
	{ "Texture", Gleam::Theme::Pink, AssetCategory::Texture },
	{ "World", Gleam::Theme::Purple, AssetCategory::World }
};

static uint32_t CategoryBit(AssetCategory category)
{
	return 1u << static_cast<uint32_t>(category);
}

ContentBrowser::ContentBrowser(EAssetManager* assetManager)
	: mAssetManager(assetManager)
{

}

void ContentBrowser::OnCreate(Gleam::Application* app)
{
	mFonts = &app->GetSubsystem<ViewStack>()->GetFonts();
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
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::PushStyleColor(ImGuiCol_WindowBg, Gleam::Theme::Panel);
		const bool visible = ImGui::Begin("Content Browser");
		ImGui::PopStyleColor();
		ImGui::PopStyleVar();

		if (visible)
		{
			if (mRefreshRequested.exchange(false, std::memory_order_relaxed))
			{
				RefreshAssetGrid();
				RefreshDirectoryTree();
			}

			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
			DrawToolbar();

			const ImVec2 bodyMin = ImGui::GetCursorScreenPos();
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(6.0f, 8.0f));
			ImGui::BeginChild("DirectoryTree", ImVec2(kTreeWidth, 0.0f), ImGuiChildFlags_AlwaysUseWindowPadding);
			ImGui::PopStyleVar();
			DrawDirectoryTree();
			ImGui::EndChild();

			const float dividerX = bodyMin.x + kTreeWidth;
			ImGui::GetWindowDrawList()->AddLine(ImVec2(dividerX, bodyMin.y), ImVec2(dividerX, bodyMin.y + ImGui::GetItemRectSize().y), Widgets::ColorU32(Gleam::Theme::Divider));

			ImGui::SameLine(0.0f, 1.0f);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12.0f, 12.0f));
			ImGui::BeginChild("AssetGrid", ImVec2(0.0f, 0.0f), ImGuiChildFlags_AlwaysUseWindowPadding);
			ImGui::PopStyleVar();
			DrawAssetGrid();
			ImGui::EndChild();
			ImGui::PopStyleVar();
		}
		ImGui::End();
	});
}

void ContentBrowser::DrawToolbar()
{
	constexpr float kButtonHeight = 30.0f;
	constexpr float kSearchWidth = 180.0f;

	const ImVec2 min = ImGui::GetCursorScreenPos();
	const float width = ImGui::GetContentRegionAvail().x;
	const float buttonY = min.y + (kToolbarHeight - kButtonHeight) * 0.5f;

	// Import
	constexpr const char* kImportLabel = ICON_LC_UPLOAD "  Import";
	ImGui::PushFont(mFonts->semiBold);
	const float importWidth = ImGui::CalcTextSize(kImportLabel).x + 24.0f;
	ImGui::SetCursorScreenPos(ImVec2(min.x + 10.0f, buttonY));
	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
	ImGui::PushStyleColor(ImGuiCol_Button, Gleam::Theme::Accent);
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Gleam::Theme::AccentHover);
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, Gleam::Theme::AccentActive);
	ImGui::PushStyleColor(ImGuiCol_Text, Gleam::Theme::OnAccent);
	if (ImGui::Button(kImportLabel, ImVec2(importWidth, kButtonHeight)))
	{
		auto files = Gleam::FileDialog::Open();
		auto importDialog = Gleam::Globals::GameInstance->GetSubsystem<ViewStack>()->GetView<AssetImportDialog>();
		importDialog->Enqueue(files, mCurrentDirectory);
	}
	ImGui::PopStyleColor(4);
	ImGui::PopStyleVar(2);
	ImGui::PopFont();

	// Search and filters
	const float searchX = min.x + width - 10.0f - kSearchWidth;
	ImGui::SetCursorScreenPos(ImVec2(searchX, buttonY));
	Widgets::SearchField("##AssetSearch", "Search assets", mSearch, sizeof(mSearch), kSearchWidth, kButtonHeight);
	const float chipsX = DrawFilterChips(searchX - 10.0f, min.y);

	// Breadcrumb
	ImGui::SetCursorScreenPos(ImVec2(min.x + 10.0f + importWidth + 10.0f, min.y));
	DrawBreadcrumb(chipsX - 10.0f);

	ImGui::SetCursorScreenPos(min);
	ImGui::Dummy(ImVec2(width, kToolbarHeight));
	ImGui::GetWindowDrawList()->AddLine(ImVec2(min.x, min.y + kToolbarHeight - 1.0f), ImVec2(min.x + width, min.y + kToolbarHeight - 1.0f), Widgets::ColorU32(Gleam::Theme::Divider));
}

void ContentBrowser::DrawBreadcrumb(float maxX)
{
	Gleam::TArray<Gleam::Path> segments;
	segments.push_back(mAssetDirectory);
	if (mCurrentDirectory != mAssetDirectory)
	{
		Gleam::Path breadcrumbPath = mAssetDirectory;
		const Gleam::Path relativePath = Gleam::Filesystem::Relative(mCurrentDirectory, mAssetDirectory);
		for (const auto& directory : relativePath.Split())
		{
			breadcrumbPath = breadcrumbPath / directory;
			segments.push_back(breadcrumbPath);
		}
	}

	auto drawList = ImGui::GetWindowDrawList();
	const ImVec2 origin = ImGui::GetCursorScreenPos();
	const float textY = origin.y + (kToolbarHeight - ImGui::GetFontSize()) * 0.5f;
	float x = origin.x;

	drawList->PushClipRect(ImVec2(origin.x, origin.y), ImVec2(maxX, origin.y + kToolbarHeight), true);
	for (uint32_t i = 0; i < segments.size(); ++i)
	{
		const bool last = i + 1 == segments.size();
		const Gleam::TString label = segments[i].Filename();
		const ImVec2 labelSize = ImGui::CalcTextSize(label.c_str());

		ImGui::PushID(static_cast<int>(i));
		ImGui::SetCursorScreenPos(ImVec2(x, textY));
		if (ImGui::InvisibleButton("##Crumb", labelSize) and last == false)
		{
			SetCurrentDir(segments[i]);
		}
		const bool hovered = ImGui::IsItemHovered() and last == false;
		ImGui::PopID();

		drawList->AddText(ImVec2(x, textY), Widgets::ColorU32(last ? Gleam::Theme::Text : (hovered ? Gleam::Theme::TextSecondary : Gleam::Theme::TextMuted)), label.c_str());
		x += labelSize.x + 6.0f;

		if (last == false)
		{
			drawList->AddText(ImVec2(x, textY), Widgets::ColorU32(Gleam::Theme::TextDim), "/");
			x += ImGui::CalcTextSize("/").x + 6.0f;
		}
	}
	drawList->PopClipRect();
}

float ContentBrowser::DrawFilterChips(float right, float top)
{
	constexpr float kChipHeight = 24.0f;
	constexpr float kChipGap = 6.0f;
	constexpr float kDotSize = 8.0f;

	ImGui::PushFont(mFonts->footnote);
	float totalWidth = 0.0f;
	for (const auto& chip : kCategoryChips)
	{
		totalWidth += 8.0f + kDotSize + 6.0f + ImGui::CalcTextSize(chip.label).x + 8.0f + kChipGap;
	}
	totalWidth -= kChipGap;

	auto drawList = ImGui::GetWindowDrawList();
	const float chipY = top + (kToolbarHeight - kChipHeight) * 0.5f;
	const float startX = right - totalWidth;
	float x = startX;

	for (const auto& chip : kCategoryChips)
	{
		const float textWidth = ImGui::CalcTextSize(chip.label).x;
		const ImVec2 min(x, chipY);
		const ImVec2 max(x + 8.0f + kDotSize + 6.0f + textWidth + 8.0f, chipY + kChipHeight);
		const uint32_t bit = CategoryBit(chip.category);
		const bool active = (mCategoryFilter & bit) != 0u;

		ImGui::SetCursorScreenPos(min);
		if (ImGui::InvisibleButton(chip.label, ImVec2(max.x - min.x, kChipHeight)))
		{
			mCategoryFilter ^= bit;
		}
		const bool hovered = ImGui::IsItemHovered();

		if (active)
		{
			drawList->AddRectFilled(min, max, Widgets::ColorU32(Gleam::Theme::Selected), kChipHeight * 0.5f);
		}
		else if (hovered)
		{
			drawList->AddRectFilled(min, max, Widgets::ColorU32(Gleam::Theme::SectionHeader), kChipHeight * 0.5f);
		}
		drawList->AddRect(min, max, Widgets::ColorU32(active ? chip.color : Gleam::Theme::Border), kChipHeight * 0.5f);
		drawList->AddCircleFilled(ImVec2(min.x + 8.0f + kDotSize * 0.5f, min.y + kChipHeight * 0.5f), kDotSize * 0.5f, Widgets::ColorU32(chip.color));
		drawList->AddText(ImVec2(min.x + 8.0f + kDotSize + 6.0f, min.y + (kChipHeight - ImGui::GetFontSize()) * 0.5f), Widgets::ColorU32(active ? Gleam::Theme::Text : Gleam::Theme::TextSecondary), chip.label);

		x = max.x + kChipGap;
	}
	ImGui::PopFont();
	return startX;
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

		if (entry.isDirectory)
		{
			entry.label = node.Filename();
			entry.color = Gleam::Theme::Accent;
		}
		else if (node.Extension() == Gleam::Asset::Extension())
		{
			entry.asset = mAssetManager->GetAsset(Gleam::Guid(node.Stem()));
			entry.label = entry.asset.name;
			entry.payloadType = "GLEAM_ASSET";

			const auto icon = GetAssetIcon(entry.asset.type);
			entry.typeText = icon.text;
			entry.color = icon.color;
			entry.category = icon.category;
		}
		else if (node.Extension() == Gleam::Prefab::Extension())
		{
			entry.asset = mAssetManager->GetAsset(Gleam::Guid(node.Stem()));
			entry.label = entry.asset.name;
			entry.payloadType = "GLEAM_PREFAB";

			const auto icon = GetAssetIcon(entry.asset.type);
			entry.typeText = icon.text;
			entry.color = icon.color;
			entry.category = icon.category;
		}
		else if (node.Extension() == Gleam::World::Extension())
		{
			entry.asset = mAssetManager->GetAsset(Gleam::Guid(node.Stem()));
			entry.label = entry.asset.name;
			entry.payloadType = "GLEAM_WORLD";

			const auto icon = GetAssetIcon(entry.asset.type);
			entry.typeText = icon.text;
			entry.color = icon.color;
			entry.category = icon.category;
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
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 2.0f));
	uint32_t index = 0u;
	while (index < mTreeEntries.size())
	{
		index = DrawDirectoryNode(index, 0u);
	}
	ImGui::PopStyleVar();
}

uint32_t ContentBrowser::DrawDirectoryNode(uint32_t index, uint32_t depth)
{
	constexpr float kChevronWidth = 12.0f;
	constexpr float kIndentWidth = 16.0f;

	const auto& entry = mTreeEntries[index];
	const uint32_t next = index + 1u + entry.descendantCount;
	const bool hasChildren = entry.descendantCount > 0u;
	const bool selected = entry.path == mCurrentDirectory;

	ImGui::PushID(static_cast<int>(index));
	auto storage = ImGui::GetStateStorage();
	const ImGuiID openID = ImGui::GetID("##Open");
	bool open = storage->GetBool(openID, depth == 0u);

	const ImVec2 rowMin = ImGui::GetCursorScreenPos();
	const float width = ImGui::GetContentRegionAvail().x;
	const ImVec2 rowMax(rowMin.x + width, rowMin.y + kTreeRowHeight);

	ImGui::SetNextItemAllowOverlap();
	if (ImGui::InvisibleButton("##Directory", ImVec2(width, kTreeRowHeight)))
	{
		SetCurrentDir(entry.path);
	}
	const bool hovered = ImGui::IsItemHovered();
	if (hovered and hasChildren and ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
	{
		open = not open;
		storage->SetBool(openID, open);
	}

	auto drawList = ImGui::GetWindowDrawList();
	if (selected)
	{
		drawList->AddRectFilled(rowMin, rowMax, Widgets::ColorU32(Gleam::Theme::Selected), 4.0f);
	}
	else if (hovered)
	{
		drawList->AddRectFilled(rowMin, rowMax, Widgets::ColorU32(Gleam::Theme::SectionHeader), 4.0f);
	}

	const float x = rowMin.x + 6.0f + depth * kIndentWidth;
	if (hasChildren)
	{
		ImGui::SetCursorScreenPos(ImVec2(x, rowMin.y));
		if (ImGui::InvisibleButton("##Toggle", ImVec2(kChevronWidth, kTreeRowHeight)))
		{
			open = not open;
			storage->SetBool(openID, open);
		}
		ImGui::PushFont(mFonts->microBold);
		Widgets::DrawIcon(drawList, open ? ICON_LC_CHEVRON_DOWN : ICON_LC_CHEVRON_RIGHT, ImVec2(x + kChevronWidth * 0.5f, rowMin.y + kTreeRowHeight * 0.5f), Gleam::Theme::TextSecondary);
		ImGui::PopFont();
	}

	drawList->AddText(ImVec2(x + kChevronWidth + 4.0f, rowMin.y + (kTreeRowHeight - ImGui::GetFontSize()) * 0.5f), Widgets::ColorU32(selected ? Gleam::Theme::Text : Gleam::Theme::TextSecondary), entry.label.c_str());
	ImGui::SetCursorScreenPos(rowMin);
	ImGui::Dummy(ImVec2(width, kTreeRowHeight));

	if (open)
	{
		uint32_t child = index + 1u;
		while (child < next)
		{
			child = DrawDirectoryNode(child, depth + 1u);
		}
	}
	ImGui::PopID();

	return next;
}

bool ContentBrowser::IsVisible(const GridEntry& entry) const
{
	if (mSearch[0] != '\0' and ImStristr(entry.label.c_str(), nullptr, mSearch, nullptr) == nullptr)
	{
		return false;
	}
	else if (mCategoryFilter != 0u)
	{
		return entry.isDirectory == false and (mCategoryFilter & CategoryBit(entry.category)) != 0u;
	}
	else
	{
		return true;
	}
}

void ContentBrowser::DrawAssetGrid()
{
	Gleam::TArray<uint32_t> visibleEntries;
	Gleam::TArray<ThumbnailItem> items;
	visibleEntries.reserve(mGridEntries.size());
	items.reserve(mGridEntries.size());
	for (uint32_t i = 0; i < mGridEntries.size(); ++i)
	{
		const auto& entry = mGridEntries[i];
		if (IsVisible(entry) == false)
		{
			continue;
		}

		visibleEntries.push_back(i);
		items.push_back({
			.label = entry.label,
			.typeText = entry.typeText,
			.color = entry.color,
			.isFolder = entry.isDirectory,
			.selected = entry.path == mSelectedEntry,
			.payloadType = entry.payloadType,
			.payload = &entry.asset,
			.payloadSize = sizeof(AssetItem)
		});
	}

	const auto events = ThumbnailGrid::Draw(items, *mFonts);
	if (events.clicked >= 0)
	{
		mSelectedEntry = mGridEntries[visibleEntries[events.clicked]].path;
	}
	if (events.doubleClicked >= 0)
	{
		const auto& entry = mGridEntries[visibleEntries[events.doubleClicked]];
		if (entry.isDirectory)
		{
			SetCurrentDir(entry.path);
		}
	}
}
