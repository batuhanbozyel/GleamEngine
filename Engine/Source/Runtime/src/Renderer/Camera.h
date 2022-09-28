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
    
    Camera(const Size& size, ProjectionType type = ProjectionType::Perspective);

    Camera(float width, float height, ProjectionType type = ProjectionType::Perspective);
    
    void Translate(const Vector3& translation);
    
    void Rotate(const Quaternion& rotation);
    
    void Rotate(const Vector3& eulers);
    
    void Rotate(float xAngle, float yAngle, float zAngle);
    
    void SetTranslation(const Vector3& translation);
    
    void SetRotation(const Quaternion& rotation);
    
    void SetViewport(const Size& size);

    void SetViewport(float width, float height);
    
    void SetProjection(ProjectionType type);
    
    void SetNearPlane(float nearPlane);
    
    void SetFarPlane(float farPlane);
    
    void SetFieldOfView(float fov);
    
    void SetSize(float size);
    
    const Matrix4& GetProjectionMatrix();
    
    const Matrix4& GetViewMatrix();
    
private:
    
    void RecalculateProjectionMatrix();
    void RecalculateViewMatrix();
    
    // Perspective projection properties
    float mFOV = 60.0f;
    
    // Orthographic projection properties
    float mSize = 5.0f;
    
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
