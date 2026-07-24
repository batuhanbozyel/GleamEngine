//
//  ContentBrowser.h
//  Editor
//
//  Created by Batuhan Bozyel on 14.11.2023.
//

#pragma once
#include "View/View.h"
#include "EAssets/AssetRegistry.h"

#include "IO/FileWatcher.h"
#include "Math/Color.h"

#include <atomic>

namespace GEditor {

class EAssetManager;

class ContentBrowser final : public View
{
public:

	ContentBrowser(EAssetManager* assetManager);

	virtual void OnCreate(Gleam::World* world) override;

	virtual void OnDestroy(Gleam::World* world) override;

    virtual void Render(Gleam::ImGuiRenderer* imgui) override;

private:

	struct GridEntry
	{
		Gleam::Path path;
		Gleam::TString label;
		AssetItem asset;
		Gleam::Color color;
		const char* iconText = nullptr;
		const char* payloadType = nullptr;
		bool isDirectory = false;
	};

	struct TreeEntry
	{
		Gleam::Path path;
		Gleam::TString label;
		uint32_t descendantCount = 0u;
	};

	bool ImportAsset(const Gleam::Path& path);

	void SetCurrentDirectory(const Gleam::Path& directory);

	void RefreshAssetGrid();

	void RefreshDirectoryTree();

	void BuildDirectoryTree(const Gleam::Path& node);

	void DrawDirectoryTree();

	uint32_t DrawDirectoryNode(uint32_t index);

	void DrawAssetGrid();

	void DrawAssetItem(const GridEntry& entry, uint32_t index, float iconSize);

	EAssetManager* mAssetManager;

    Gleam::Path mCurrentDirectory;

	Gleam::Path mAssetDirectory;

	// Navigation requested from inside the grid loop, applied once the loop is done
	Gleam::Path mPendingDirectory;

	Gleam::TArray<GridEntry> mGridEntries;

	Gleam::TArray<TreeEntry> mTreeEntries;

	Gleam::FileWatcher::Handle* mWatchHandle = nullptr;

	// Set from the FileWatcher's background thread, consumed during Render
	std::atomic<bool> mRefreshRequested = false;

};

} // namespace GEditor
