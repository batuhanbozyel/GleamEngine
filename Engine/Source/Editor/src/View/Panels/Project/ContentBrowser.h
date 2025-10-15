//
//  ContentBrowser.h
//  Editor
//
//  Created by Batuhan Bozyel on 14.11.2023.
//

#pragma once
#include "View/View.h"
#include "EAssets/EAssetManager.h"

namespace GEditor {

class ContentBrowser final : public View
{
public:
    
	virtual void Init(Gleam::World* world) override;
    
    virtual void Render(Gleam::ImGuiRenderer* imgui) override;
    
private:

	bool ImportAsset(const Gleam::Path& path);
    
    void DrawDirectoryTreeView(const Gleam::Path& directory);
    
    EAssetManager* mAssetManager;

    Gleam::Path mCurrentDirectory;
    
	Gleam::Path mAssetDirectory;
    
};

} // namespace GEditor
