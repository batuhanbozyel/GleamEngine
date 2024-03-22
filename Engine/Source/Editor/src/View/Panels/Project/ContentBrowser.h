//
//  ContentBrowser.h
//  Editor
//
//  Created by Batuhan Bozyel on 14.11.2023.
//

#pragma once
#include "Gleam.h"
#include "View/View.h"
#include "EAssets/AssetManager.h"

namespace GEditor {

class ContentBrowser final : public View
{
public:
    
    ContentBrowser();
    
    virtual void Render(Gleam::ImGuiRenderer* imgui) override;
    
private:

	bool ImportAsset(const Gleam::Filesystem::path& path);
    
    void DrawDirectoryTreeView(const Gleam::Filesystem::path& directory);
    
    AssetManager mAssetManager;

    Gleam::Filesystem::path mCurrentDirectory;
    
	Gleam::Filesystem::path mAssetDirectory;
    
};

} // namespace GEditor
