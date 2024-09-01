//
//  ContentBrowser.h
//  Editor
//
//  Created by Batuhan Bozyel on 14.11.2023.
//

#pragma once
#include "Gleam.h"
#include "View/View.h"
#include "EAssets/AssetRegistry.h"

namespace GEditor {

class ContentBrowser final : public View
{
public:
    
    ContentBrowser();
    
    virtual void Render(Gleam::ImGuiRenderer* imgui) override;
    
private:

	bool ImportAsset(const Gleam::Filesystem::Path& path);
    
    void DrawDirectoryTreeView(const Gleam::Filesystem::Path& directory);
    
    AssetRegistry mAssetRegistry;

    Gleam::Filesystem::Path mCurrentDirectory;
    
	Gleam::Filesystem::Path mAssetDirectory;
    
};

} // namespace GEditor
