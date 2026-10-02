//
//  MenuBar.h
//  Editor
//
//  Created by Batuhan Bozyel on 19.05.2023.
//

#pragma once
#include "View/View.h"

namespace Gleam {
class World;
} // namespace Gleam

namespace GEditor {

struct EditorFonts;

class MenuBar final : public View
{
public:

	MenuBar(Gleam::World* world);

	virtual void OnCreate(Gleam::Application* app) override;

    virtual void Render(Gleam::ImGuiRenderer* imgui) override;
    
private:

	Gleam::World* mWorld = nullptr;

	Gleam::Application* mApplication = nullptr;

	const EditorFonts* mFonts = nullptr;

};

} // namespace GEditor
