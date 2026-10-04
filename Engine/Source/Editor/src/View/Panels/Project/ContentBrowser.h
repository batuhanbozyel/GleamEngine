//
//  ContentBrowser.h
//  Editor
//
//  Created by Batuhan Bozyel on 14.11.2023.
//

#pragma once
#include "View/View.h"
#include "View/Widgets/AssetIcon.h"
#include "EAssets/AssetRegistry.h"

#include "IO/FileWatcher.h"

#include <atomic>

namespace GEditor {

class EAssetManager;
struct EditorFonts;

class ContentBrowser final : public View
{
public:

	ContentBrowser(EAssetManager* assetManager);

	virtual void OnCreate(Gleam::Application* app) override;

	virtual void OnDestroy(Gleam::Application* app) override;

    virtual void Render(Gleam::ImGuiRenderer* imgui) override;

private:

	struct GridEntry
	{
		Gleam::Path path;
		Gleam::TString label;
		AssetItem asset;
		ImVec4 color = Gleam::Theme::TextDim;
		const char* typeText = nullptr;
		AssetCategory category = AssetCategory::Other;
		const char* payloadType = nullptr;
		bool isDirectory = false;
	};

	struct TreeEntry
	{
		Gleam::Path path;
		Gleam::TString label;
		uint32_t descendantCount = 0u;
	};

	void SetCurrentDir(const Gleam::Path& directory);

	void RefreshAssetGrid();

	void RefreshDirectoryTree();

	void BuildDirectoryTree(const Gleam::Path& node);

	void DrawToolbar();

	void DrawBreadcrumb(float maxX);

	float DrawFilterChips(float right, float top);

	void DrawDirectoryTree();

	uint32_t DrawDirectoryNode(uint32_t index, uint32_t depth);

	void DrawAssetGrid();

	bool IsVisible(const GridEntry& entry) const;

	EAssetManager* mAssetManager;

	const EditorFonts* mFonts = nullptr;

    Gleam::Path mCurrentDirectory;

	Gleam::Path mAssetDirectory;

	Gleam::TArray<GridEntry> mGridEntries;

	Gleam::TArray<TreeEntry> mTreeEntries;

	Gleam::Path mSelectedEntry;

	uint32_t mCategoryFilter = 0u;

	char mSearch[128] = "";

	Gleam::FileWatcher::Handle* mWatchHandle = nullptr;

	// Set from the FileWatcher's background thread, consumed during Render
	std::atomic<bool> mRefreshRequested = false;

};

} // namespace GEditor
