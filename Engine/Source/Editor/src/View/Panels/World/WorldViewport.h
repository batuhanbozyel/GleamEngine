//
//  WorldViewport.h
//  Editor
//
//  Created by Batuhan Bozyel on 26.03.2023.
//

#pragma once
#include "View/View.h"
#include "Gleam.h"

namespace GEditor {

class EditorCameraController;

class WorldViewport final : public View
{
public:
    
    WorldViewport(Gleam::World* world);
    
    virtual void Update() override;
    
    virtual void Render(Gleam::ImGuiRenderer* imgui) override;
    
    const Gleam::Size& GetViewportSize() const
    {
        return mViewportSize;
    }
    
private:
    
    bool mIsFocused = false;
    
    bool mCursorVisible = true;

	bool mViewportSizeChanged = false;
    
    EditorCameraController* mController = nullptr;
    
    Gleam::Size mViewportSize;
    
    Gleam::World* mEditWorld;
    
};

} // namespace GEditor
