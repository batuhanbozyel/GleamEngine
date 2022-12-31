#pragma once
#include "Gleam.h"

namespace GEditor {

class SceneView : public Gleam::View
{
public:
    
    virtual void OnCreate() override;
    
    virtual void OnUpdate() override;
    
    virtual void OnRender() override;

private:
    
    bool mCursorVisible = true;
    
    float mYaw = 0.0f;
    float mPitch = 0.0f;
    
    Gleam::World mWorld;
    Gleam::Camera mCamera;
    Gleam::TArray<Gleam::BoundingBox> mBounds;
    Gleam::TArray<Gleam::Scope<Gleam::Mesh>> mMeshes;
};
    
} // namespace GEditor
