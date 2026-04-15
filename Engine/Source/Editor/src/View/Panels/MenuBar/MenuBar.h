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

	virtual void OnCreate(Gleam::World* world) override;

    virtual void Render(Gleam::ImGuiRenderer* imgui) override;
    
private:

	Gleam::World* mWorld = nullptr;

};

} // namespace GEditor
