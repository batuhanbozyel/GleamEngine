#pragma once

namespace Gleam {

class Transform;

enum class ProjectionType
{
	Ortho,
	Perspective
};

class Camera final : public Transform
{
public:
    
	Camera() = default;

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
    
    void SetOrthographicSize(float size);
    
    const Matrix4& GetProjectionMatrix() const;
    
    const Matrix4& GetViewMatrix() const;
    
private:
    
    void RecalculateProjectionMatrix() const;
    void RecalculateViewMatrix() const;
    
    // Perspective projection properties
    float mFOV = 60.0f;
    
    // Orthographic projection properties
    float mOrthographicSize = 5.0f;
    
    // Common properties
    float mAspectRatio = 1.0f;
    float mNearPlane = 0.1f;
    float mFarPlane = 1000.0f;
    
	ProjectionType mProjectionType = ProjectionType::Perspective;
    
    mutable Matrix4 mViewMatrix;
    mutable Matrix4 mProjectionMatrix;
    
    // Update flags
    mutable bool mViewMatrixDirty = true;
    mutable bool mProjectionMatrixDirty = true;
};

} // namespace Gleam
