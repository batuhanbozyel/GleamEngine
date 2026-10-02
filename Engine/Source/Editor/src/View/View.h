//
//  View.h
//  Editor
//
//  Created by Batuhan Bozyel on 27.03.2023.
//

#pragma once

namespace Gleam {
class Application;
class ImGuiRenderer;
} // namespace Gleam

namespace GEditor {

class View
{
public:
    
    virtual ~View() = default;

	virtual void OnCreate(Gleam::Application* app) {}
	
	virtual void OnDestroy(Gleam::Application* app) {}
    
    virtual void Update() {}
    
    virtual void Render(Gleam::ImGuiRenderer* imgui) {}
    
};

} // namespace GEditor
