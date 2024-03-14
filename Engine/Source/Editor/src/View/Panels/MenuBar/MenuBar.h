//
//  MenuBar.h
//  Editor
//
//  Created by Batuhan Bozyel on 19.05.2023.
//

#pragma once
#include "View/View.h"

namespace GEditor {

class MenuBar final : public View
{
public:
    
    MenuBar();
    
    virtual void Render(Gleam::ImGuiRenderer* imgui) override;
    
private:


    
};

} // namespace GEditor
