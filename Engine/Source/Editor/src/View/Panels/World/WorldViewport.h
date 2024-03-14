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

class WorldViewportController;

class WorldViewport final : public View
{
public:
    
    WorldViewport();
    
    virtual void Update() override;
    
    virtual void Render(Gleam::ImGuiRenderer* imgui) override;
    
    const Gleam::Size& GetViewportSize() const
    {
        return mViewportSize;
    }
    
    const Gleam::RefCounted<Gleam::World>& GetWorld() const
    {
        return mEditWorld;
    }
    
private:
    
    bool mIsFocused = false;
    
    bool mCursorVisible = false;
    
    WorldViewportController* mController = nullptr;
    
    Gleam::Size mViewportSize;
    
    Gleam::RefCounted<Gleam::World> mEditWorld;
    
};

} // namespace GEditor
