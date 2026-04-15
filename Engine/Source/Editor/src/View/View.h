//
//  View.h
//  Editor
//
//  Created by Batuhan Bozyel on 27.03.2023.
//

#pragma once
#include "Events/EntityEvent.h"

namespace Gleam {
class ImGuiRenderer;
class World;
} // namespace Gleam

namespace GEditor {

class View
{
public:
    
    virtual ~View() = default;

	virtual void OnCreate(Gleam::World* world) {}
	
	virtual void OnDestroy(Gleam::World* world) {}
    
    virtual void Update() {}
    
    virtual void Render(Gleam::ImGuiRenderer* imgui) {}
    
};

} // namespace GEditor
