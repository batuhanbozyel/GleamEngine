#pragma once
#include "../Scene/Component.h"

namespace Gleam {

enum class ProjectionType
{
	Ortho,
	Perspective
};

class Camera : public Component
{
public:
    
    COMPONENT_BODY();
    
    void SetViewport(float width, float height);
    
    void SetProjection(ProjectionType type);
    
    void Update();
    
    const Matrix4& GetProjectionMatrix() const;
    
    const Matrix4& GetViewMatrix() const;
    
private:
    
    void RecalculateProjectionMatrix();
    void RecalculateViewMatrix();
    
    // Perspective projection properties
    float mFOV = 60.0f;
    
    // Orthographic projection properties
    float mSize = 1.0f;
    
    // Common properties
    float mAspectRatio = 1.0f;
    float mNearPlane = 0.3f;
    float mFarPlane = 1000.0f;
    
    Matrix4 mViewMatrix;
    Matrix4 mProjectionMatrix;
	ProjectionType mProjectionType;
    
    // Update flags
    bool mProjectionMatrixDirty = true;
};

} // namespace Gleam
