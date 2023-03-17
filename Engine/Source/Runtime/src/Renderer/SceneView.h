#pragma once
#include "View.h"
#include "World/World.h"

namespace Gleam {

class Mesh;

class SceneView : public View
{
public:
    
    GLEAM_VIEW_CLASS(SceneView);

    virtual void OnCreate() override;
    
    virtual void OnUpdate() override;
    
    virtual void OnRender() override;
    
private:
    
    bool mCursorVisible = true;
    
    float mYaw = 0.0f;
    float mPitch = 0.0f;
    
    RefCounted<World> mWorld;

};

} // namespace Gleam
