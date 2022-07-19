#pragma once

namespace Gleam {

class Transform;

enum class ProjectionType
{
	Ortho,
	Perspective
};

class Camera : public Transform
{
public:
    
    Camera();

	Camera(const Vector2& size, ProjectionType type = ProjectionType::Perspective);

    Camera(float width, float height, ProjectionType type = ProjectionType::Perspective);

	void SetViewport(const Vector2& size);

    void SetViewport(float width, float height);
    
    void SetProjection(ProjectionType type);
    
    void Translate(const Vector3& translation);
    
    const Matrix4& GetProjectionMatrix();
    
    const Matrix4& GetViewMatrix();
    
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
    bool mViewMatrixDirty = true;
    bool mProjectionMatrixDirty = true;
};

} // namespace Gleam
