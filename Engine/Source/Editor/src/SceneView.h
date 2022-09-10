#pragma once
#include "Gleam.h"
#include "Renderer/DebugRenderer.h"

namespace GEditor {
    
class SceneView : public Gleam::Layer
{
public:
    
    virtual void OnAttach() override;
    
    virtual void OnUpdate() override;
    
    virtual void OnRender() override;

private:
    
    bool mCursorVisible = true;
    
    float mYaw = 0.0f;
    float mPitch = 0.0f;
    
    Gleam::World mWorld;
    Gleam::Camera mCamera;
    Gleam::DebugRenderer mRenderer;
    Gleam::TArray<Gleam::BoundingBox> mBounds;
    Gleam::TArray<Gleam::Scope<Gleam::Mesh>> mMeshes;
};
    
} // namespace GEditor
