//
//  View.h
//  Editor
//
//  Created by Batuhan Bozyel on 27.03.2023.
//

#pragma once
#include "Gleam.h"
#include "Events/EntityEvent.h"

namespace GEditor {

class View
{
public:
    
    virtual ~View() = default;

	virtual void Init(Gleam::World* world) {}
    
    virtual void Update() {}
    
    virtual void Render(Gleam::ImGuiRenderer* imgui) {}
    
};

} // namespace GEditor
