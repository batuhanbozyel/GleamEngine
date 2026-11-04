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
#include "View/Panels/Input/InputActionsEditor.h"
#include "View/Panels/Rendering/RenderPipelineEditor.h"
#include "View/Panels/Rendering/RenderTextureEditor.h"
#include "EInput/EditorInputSystem.h"
#include "Input/InputActionDescriptor.h"
#include "Renderer/GraphicsSettings.h"
#include "Renderer/RenderPipelineDescriptor.h"
#include "Assets/AssetManager.h"
#include "Core/ConfigSystem.h"
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

#include <cstdio>

#include <imgui_internal.h>

#include <cstring>

using namespace GEditor;

static constexpr float kToolbarHeight = 44.0f;
static constexpr float kTreeWidth = 180.0f;
static constexpr float kTreeRowHeight = 26.0f;

struct CategoryFilter
{
	const char* label;
	ImVec4 color;
	AssetCategory category;
};

static constexpr CategoryFilter kCategoryFilters[] = {
	{ "Prefab", Gleam::Theme::Warning, AssetCategory::Prefab },
	{ "Mesh", Gleam::Theme::Info, AssetCategory::Mesh },
	{ "Material", Gleam::Theme::Success, AssetCategory::Material },
	{ "Material Instance", Gleam::Theme::Teal, AssetCategory::MaterialInstance },
	{ "Texture", Gleam::Theme::Pink, AssetCategory::Texture },
	{ "World", Gleam::Theme::Purple, AssetCategory::World },
	{ "Input Actions", Gleam::Theme::Accent, AssetCategory::InputActions },
	{ "Render Pipeline", Gleam::Theme::Lime, AssetCategory::RenderPipeline },
	{ "Render Texture", Gleam::Theme::Pink, AssetCategory::RenderTexture }
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
	mEditorInput = app->GetSubsystem<EditorInputSystem>();
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

		const bool focused = visible and ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
		if (focused)
		{
			mEditorInput->RequestPanelActions();
		}

		if (visible)
		{
			if (mRefreshRequested.exchange(false, std::memory_order_relaxed))
			{
				RefreshAssetGrid();
				RefreshDirectoryTree();
			}

			mItemSpacing = ImGui::GetStyle().ItemSpacing;
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
	const float filterX = DrawTypeFilter(searchX - 8.0f, min.y);

	// Breadcrumb
	ImGui::SetCursorScreenPos(ImVec2(min.x + 10.0f + importWidth + 10.0f, min.y));
	DrawBreadcrumb(filterX - 10.0f);

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

float ContentBrowser::DrawTypeFilter(float right, float top)
{
	constexpr float kButtonHeight = 30.0f;
	constexpr float kPaddingX = 10.0f;
	constexpr float kRowHeight = 28.0f;
	constexpr float kPopupWidth = 220.0f;
	constexpr float kMaxListHeight = 320.0f;
	constexpr float kCheckSize = 14.0f;
	constexpr float kDotSize = 8.0f;

	uint32_t activeCount = 0u;
	const CategoryFilter* firstActive = nullptr;
	for (const auto& filter : kCategoryFilters)
	{
		if ((mCategoryFilter & CategoryBit(filter.category)) != 0u)
		{
			firstActive = firstActive ? firstActive : &filter;
			++activeCount;
		}
	}

	char label[32];
	if (activeCount == 0u)
	{
		std::snprintf(label, sizeof(label), "All types");
	}
	else if (activeCount == 1u)
	{
		std::snprintf(label, sizeof(label), "%s", firstActive->label);
	}
	else
	{
		std::snprintf(label, sizeof(label), "%u types", activeCount);
	}

	const float iconWidth = ImGui::CalcTextSize(ICON_LC_LIST_FILTER).x;
	const float chevronWidth = ImGui::CalcTextSize(ICON_LC_CHEVRON_DOWN).x;
	const float labelWidth = ImGui::CalcTextSize(label).x;
	const float width = kPaddingX + iconWidth + 6.0f + labelWidth + 8.0f + chevronWidth + kPaddingX;

	const ImVec2 min(right - width, top + (kToolbarHeight - kButtonHeight) * 0.5f);
	const ImVec2 max(right, min.y + kButtonHeight);
	ImGui::SetCursorScreenPos(min);
	if (ImGui::InvisibleButton("##TypeFilter", ImVec2(width, kButtonHeight)))
	{
		ImGui::OpenPopup("##TypeFilterPopup");
	}
	const bool hovered = ImGui::IsItemHovered();
	const bool open = ImGui::IsPopupOpen("##TypeFilterPopup");
	const bool filtering = activeCount > 0u;

	auto drawList = ImGui::GetWindowDrawList();
	const float centerY = min.y + kButtonHeight * 0.5f;
	drawList->AddRectFilled(min, max, Widgets::ColorU32(hovered or open ? Gleam::Theme::ControlHover : Gleam::Theme::Background), 6.0f);
	drawList->AddRect(min, max, Widgets::ColorU32(filtering ? Gleam::Theme::Accent : Gleam::Theme::Border), 6.0f);
	Widgets::DrawIcon(drawList, ICON_LC_LIST_FILTER, ImVec2(min.x + kPaddingX + iconWidth * 0.5f, centerY), filtering ? Gleam::Theme::Accent : Gleam::Theme::TextDim);
	drawList->AddText(ImVec2(min.x + kPaddingX + iconWidth + 6.0f, IM_ROUND(centerY - ImGui::GetFontSize() * 0.5f)), Widgets::ColorU32(filtering ? Gleam::Theme::Text : Gleam::Theme::TextSecondary), label);
	Widgets::DrawIcon(drawList, ICON_LC_CHEVRON_DOWN, ImVec2(max.x - kPaddingX - chevronWidth * 0.5f, centerY), Gleam::Theme::TextDim);

	ImGui::SetNextWindowPos(ImVec2(max.x, max.y + 4.0f), ImGuiCond_Always, ImVec2(1.0f, 0.0f));
	ImGui::SetNextWindowSize(ImVec2(kPopupWidth, 0.0f));
	if (ImGui::BeginPopup("##TypeFilterPopup"))
	{
		ImGui::PushFont(mFonts->microBold);
		ImGui::AlignTextToFramePadding();
		ImGui::TextColored(Gleam::Theme::TextDim, "ASSET TYPES");
		ImGui::PopFont();
		if (filtering)
		{
			constexpr const char* kClearLabel = "Clear";
			ImGui::SameLine(ImGui::GetContentRegionMax().x - ImGui::CalcTextSize(kClearLabel).x);
			const ImVec2 clearPos = ImGui::GetCursorScreenPos();
			if (ImGui::InvisibleButton("##Clear", ImGui::CalcTextSize(kClearLabel)))
			{
				mCategoryFilter = 0u;
			}
			ImGui::GetWindowDrawList()->AddText(clearPos, Widgets::ColorU32(ImGui::IsItemHovered() ? Gleam::Theme::Text : Gleam::Theme::Info), kClearLabel);
		}
		ImGui::Separator();

		const float rowSpacing = ImGui::GetStyle().ItemSpacing.y;
		const float contentHeight = IM_ARRAYSIZE(kCategoryFilters) * (kRowHeight + rowSpacing) - rowSpacing;
		// The browser usually docks at the bottom, so the list also stops at the bottom of the window
		const auto viewport = ImGui::GetWindowViewport();
		const float spaceBelow = viewport->WorkPos.y + viewport->WorkSize.y - ImGui::GetCursorScreenPos().y - ImGui::GetStyle().WindowPadding.y * 2.0f;
		const float listHeight = Gleam::Math::Max(Gleam::Math::Min(contentHeight, Gleam::Math::Min(kMaxListHeight, spaceBelow)), kRowHeight);
		ImGui::BeginChild("##TypeList", ImVec2(0.0f, listHeight));
		for (const auto& filter : kCategoryFilters)
		{
			const uint32_t bit = CategoryBit(filter.category);
			const bool active = (mCategoryFilter & bit) != 0u;

			const ImVec2 rowMin = ImGui::GetCursorScreenPos();
			ImGui::PushID(filter.label);
			if (ImGui::Selectable("##Row", false, ImGuiSelectableFlags_NoAutoClosePopups, ImVec2(0.0f, kRowHeight)))
			{
				mCategoryFilter ^= bit;
			}
			ImGui::PopID();

			auto popupDrawList = ImGui::GetWindowDrawList();
			const float rowCenterY = rowMin.y + kRowHeight * 0.5f;
			const ImVec2 checkMin(rowMin.x + 2.0f, rowCenterY - kCheckSize * 0.5f);
			const ImVec2 checkMax(checkMin.x + kCheckSize, checkMin.y + kCheckSize);
			popupDrawList->AddRectFilled(checkMin, checkMax, Widgets::ColorU32(active ? Gleam::Theme::Accent : Gleam::Theme::Background), 3.0f);
			popupDrawList->AddRect(checkMin, checkMax, Widgets::ColorU32(active ? Gleam::Theme::Accent : Gleam::Theme::Border), 3.0f);
			if (active)
			{
				Widgets::DrawIcon(popupDrawList, ICON_LC_CHECK, ImVec2(checkMin.x + kCheckSize * 0.5f, rowCenterY), Gleam::Theme::OnAccent);
			}
			popupDrawList->AddCircleFilled(ImVec2(checkMax.x + 10.0f + kDotSize * 0.5f, rowCenterY), kDotSize * 0.5f, Widgets::ColorU32(filter.color));
			popupDrawList->AddText(ImVec2(checkMax.x + 10.0f + kDotSize + 8.0f, IM_ROUND(rowCenterY - ImGui::GetFontSize() * 0.5f)), Widgets::ColorU32(active ? Gleam::Theme::Text : Gleam::Theme::TextSecondary), filter.label);
		}
		ImGui::EndChild();
		ImGui::EndPopup();
	}
	return min.x;
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
	if (events.contextClicked >= 0)
	{
		mContextEntry = mGridEntries[visibleEntries[events.contextClicked]];
		mSelectedEntry = mContextEntry.path;
		ImGui::OpenPopup("AssetItemContext");
	}
	if (events.doubleClicked >= 0)
	{
		const auto& entry = mGridEntries[visibleEntries[events.doubleClicked]];
		if (entry.isDirectory)
		{
			SetCurrentDir(entry.path);
		}
		else if (entry.category == AssetCategory::InputActions)
		{
			auto viewStack = Gleam::Globals::GameInstance->GetSubsystem<ViewStack>();
			viewStack->GetView<InputActionsEditor>()->Open(entry.asset, entry.path);
		}
		else if (entry.category == AssetCategory::RenderPipeline)
		{
			auto viewStack = Gleam::Globals::GameInstance->GetSubsystem<ViewStack>();
			viewStack->GetView<RenderPipelineEditor>()->Open(entry.asset, entry.path);
		}
		else if (entry.category == AssetCategory::RenderTexture)
		{
			auto viewStack = Gleam::Globals::GameInstance->GetSubsystem<ViewStack>();
			viewStack->GetView<RenderTextureEditor>()->Open(entry.asset, entry.path);
		}
	}

	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, mItemSpacing);
	if (events.contextClicked < 0 and ImGui::IsWindowHovered() and ImGui::IsMouseReleased(ImGuiMouseButton_Right))
	{
		ImGui::OpenPopup("AssetGridContext");
	}
	if (ImGui::BeginPopup("AssetGridContext"))
	{
		if (ImGui::BeginMenu("Create", mAssetManager->IsBuiltinAsset(mCurrentDirectory / "_") == false))
		{
			if (ImGui::MenuItem("Input Actions"))
			{
				Gleam::InputActionMapDescriptor map;
				map.id = Gleam::Guid::NewGuid();
				map.name = "Player";

				Gleam::InputActionAssetDescriptor descriptor;
				descriptor.maps.push_back(map);

				const auto& asset = mAssetManager->CreateAsset(mCurrentDirectory, "New Input Actions", descriptor);
				auto fileName = asset.reference.guid.ToString();
				fileName.append(".asset");
				mSelectedEntry = mCurrentDirectory / fileName;
				RefreshAssetGrid();

				auto viewStack = Gleam::Globals::GameInstance->GetSubsystem<ViewStack>();
				viewStack->GetView<InputActionsEditor>()->Open(asset, mSelectedEntry);
			}
			if (ImGui::MenuItem("Render Pipeline"))
			{
				const auto& graphics = Gleam::Globals::Engine->GetSubsystem<Gleam::ConfigSystem>()->Get<Gleam::GraphicsSettings>();
				const auto descriptor = Gleam::Globals::GameInstance->GetSubsystem<Gleam::AssetManager>()->LoadDescriptor<Gleam::RenderPipelineDescriptor>(graphics.renderPipeline);

				const auto& asset = mAssetManager->CreateAsset(mCurrentDirectory, "New Render Pipeline", descriptor);
				auto fileName = asset.reference.guid.ToString();
				fileName.append(".asset");
				mSelectedEntry = mCurrentDirectory / fileName;
				RefreshAssetGrid();

				auto viewStack = Gleam::Globals::GameInstance->GetSubsystem<ViewStack>();
				viewStack->GetView<RenderPipelineEditor>()->Open(asset, mSelectedEntry);
			}
			if (ImGui::MenuItem("Render Texture"))
			{
				Gleam::RenderTextureDescriptor descriptor;
				descriptor.size = Gleam::Size(1024.0f, 1024.0f);

				const auto& asset = mAssetManager->CreateAsset(mCurrentDirectory, "New Render Texture", descriptor);
				auto fileName = asset.reference.guid.ToString();
				fileName.append(".asset");
				mSelectedEntry = mCurrentDirectory / fileName;
				RefreshAssetGrid();

				auto viewStack = Gleam::Globals::GameInstance->GetSubsystem<ViewStack>();
				viewStack->GetView<RenderTextureEditor>()->Open(asset, mSelectedEntry);
			}
			ImGui::EndMenu();
		}
		ImGui::EndPopup();
	}

	const auto& actions = mEditorInput->GetPanelActions();
	const auto selected = FindSelectedEntry();
	if (selected and ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows))
	{
		if (mEditorInput->GetAction(actions.rename).performedThisFrame)
		{
			RequestRename(*selected);
		}
		else if (mEditorInput->GetAction(actions.remove).performedThisFrame)
		{
			RequestDelete(*selected);
		}
	}

	DrawAssetContextMenu();
	DrawRenameDialog();
	DrawDeleteDialog();
	ImGui::PopStyleVar();
}

const ContentBrowser::GridEntry* ContentBrowser::FindSelectedEntry() const
{
	for (const auto& entry : mGridEntries)
	{
		if (entry.path == mSelectedEntry)
		{
			return &entry;
		}
	}
	return nullptr;
}

bool ContentBrowser::IsEditable(const GridEntry& entry) const
{
	return (entry.category == AssetCategory::InputActions or entry.category == AssetCategory::RenderPipeline or entry.category == AssetCategory::RenderTexture) and mAssetManager->IsBuiltinAsset(entry.path) == false;
}

void ContentBrowser::RequestRename(const GridEntry& entry)
{
	if (IsEditable(entry) == false)
	{
		return;
	}

	mContextEntry = entry;
	const auto length = Gleam::Math::Min(mContextEntry.asset.name.size(), sizeof(mRenameBuffer) - 1);
	std::memcpy(mRenameBuffer, mContextEntry.asset.name.c_str(), length);
	mRenameBuffer[length] = '\0';
	mRenameRequested = true;
}

void ContentBrowser::RequestDelete(const GridEntry& entry)
{
	if (IsEditable(entry) == false)
	{
		return;
	}

	mContextEntry = entry;
	mDeleteRequested = true;
}

void ContentBrowser::DrawAssetContextMenu()
{
	if (ImGui::BeginPopup("AssetItemContext"))
	{
		const auto& actions = mEditorInput->GetPanelActions();
		const bool editable = IsEditable(mContextEntry);

		if (ImGui::MenuItem("Rename", mEditorInput->GetShortcutLabel(actions.rename).c_str(), false, editable))
		{
			RequestRename(mContextEntry);
		}
		if (ImGui::MenuItem("Delete", mEditorInput->GetShortcutLabel(actions.remove).c_str(), false, editable))
		{
			RequestDelete(mContextEntry);
		}
		ImGui::EndPopup();
	}

	if (mRenameRequested)
	{
		ImGui::OpenPopup("Rename Asset");
		mRenameRequested = false;
	}
	if (mDeleteRequested)
	{
		ImGui::OpenPopup("Delete Asset");
		mDeleteRequested = false;
	}
}

void ContentBrowser::DrawRenameDialog()
{
	if (ImGui::BeginPopupModal("Rename Asset", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
	{
		if (ImGui::IsWindowAppearing())
		{
			ImGui::SetKeyboardFocusHere();
		}

		ImGui::SetNextItemWidth(280.0f);
		const bool submitted = ImGui::InputText("##Name", mRenameBuffer, sizeof(mRenameBuffer), ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll);
		const bool valid = mAssetManager->IsValidAssetName(mContextEntry.path.Parent(), mContextEntry.asset, mRenameBuffer);
		if (valid == false)
		{
			ImGui::TextColored(Gleam::Theme::Error, "Name is empty, contains a path separator or is already taken");
		}

		ImGui::BeginDisabled(valid == false);
		const bool confirmed = ImGui::Button("Rename", ImVec2(120.0f, 0.0f)) or (submitted and valid);
		ImGui::EndDisabled();
		ImGui::SameLine();
		const bool canceled = ImGui::Button("Cancel", ImVec2(120.0f, 0.0f)) or ImGui::IsKeyPressed(ImGuiKey_Escape, false);

		if (confirmed)
		{
			if (mContextEntry.asset.name != Gleam::TStringView(mRenameBuffer))
			{
				if (mAssetManager->RenameAsset(mContextEntry.path, mContextEntry.asset, mRenameBuffer))
				{
					auto viewStack = Gleam::Globals::GameInstance->GetSubsystem<ViewStack>();
					viewStack->GetView<InputActionsEditor>()->OnAssetRenamed(mContextEntry.asset);
					viewStack->GetView<RenderPipelineEditor>()->OnAssetRenamed(mContextEntry.asset);
					viewStack->GetView<RenderTextureEditor>()->OnAssetRenamed(mContextEntry.asset);
					RefreshAssetGrid();
				}
			}
			ImGui::CloseCurrentPopup();
		}
		else if (canceled)
		{
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}
}

void ContentBrowser::DrawDeleteDialog()
{
	if (ImGui::BeginPopupModal("Delete Asset", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::Text("Delete \"%s\"? This cannot be undone.", mContextEntry.asset.name.c_str());

		const bool confirmed = Widgets::AccentButton("Delete", ImVec2(120.0f, 0.0f));
		ImGui::SameLine();
		const bool canceled = ImGui::Button("Cancel", ImVec2(120.0f, 0.0f)) or ImGui::IsKeyPressed(ImGuiKey_Escape, false);

		if (confirmed)
		{
			if (mAssetManager->RemoveAsset(mContextEntry.path, mContextEntry.asset))
			{
				auto viewStack = Gleam::Globals::GameInstance->GetSubsystem<ViewStack>();
				viewStack->GetView<InputActionsEditor>()->OnAssetRemoved(mContextEntry.asset.reference.guid);
				viewStack->GetView<RenderPipelineEditor>()->OnAssetRemoved(mContextEntry.asset.reference.guid);
				viewStack->GetView<RenderTextureEditor>()->OnAssetRemoved(mContextEntry.asset.reference.guid);
				if (mSelectedEntry == mContextEntry.path)
				{
					mSelectedEntry = Gleam::Path();
				}
				RefreshAssetGrid();
			}
			ImGui::CloseCurrentPopup();
		}
		else if (canceled)
		{
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}
}
