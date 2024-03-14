//
//  View.h
//  Editor
//
//  Created by Batuhan Bozyel on 27.03.2023.
//

#pragma once
#include "Gleam.h"

namespace GEditor {

class View
{
public:
    
    virtual ~View() = default;
    
    virtual void Update() {}
    
    virtual void Render(Gleam::ImGuiRenderer* imgui) {}
    
};

} // namespace GEditor
