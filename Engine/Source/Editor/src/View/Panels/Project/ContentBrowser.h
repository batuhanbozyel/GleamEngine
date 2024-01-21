//
//  ContentBrowser.h
//  Editor
//
//  Created by Batuhan Bozyel on 14.11.2023.
//

#pragma once
#include "View/View.h"
#include "Gleam.h"

namespace GEditor {

class ContentBrowser final : public View
{
public:
    
    ContentBrowser();
    
    virtual void Render() override;
    
private:

	Gleam::Filesystem::path mAssetDirectory;
    
};

} // namespace GEditor
