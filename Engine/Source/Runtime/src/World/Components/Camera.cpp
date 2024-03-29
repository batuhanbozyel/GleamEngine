#include "gpch.h"
#include "Camera.h"
#include "Transform.h"

using namespace Gleam;

Camera::Camera(float width, float height, ProjectionType type)
	: Camera({width, height}, type)
{

}

Camera::Camera(const Size& size, ProjectionType type)
	: mProjectionType(type)
{
	SetViewport(size);
}

void Camera::Translate(const Vector3& translation)
{
    mViewMatrixDirty = true;
    Transform::Translate(translation);
}

void Camera::Rotate(const Quaternion& rotation)
{
    mViewMatrixDirty = true;
    Transform::Rotate(rotation);
}

void Camera::Rotate(const Vector3& eulers)
{
    Rotate(Quaternion(eulers));
}

void Camera::Rotate(float xAngle, float yAngle, float zAngle)
{
    Rotate(Vector3{xAngle, yAngle, zAngle});
}

void Camera::SetTranslation(const Vector3& translation)
{
    mViewMatrixDirty = true;
    Transform::SetTranslation(translation);
}

void Camera::SetRotation(const Quaternion& rotation)
{
    mViewMatrixDirty = true;
    Transform::SetRotation(rotation);
}

void Camera::SetViewport(const Size& size)
{
	SetViewport(size.width, size.height);
}

void Camera::SetViewport(float width, float height)
{
	float aspectRatio = width / height;
    mAspectRatio = aspectRatio;
    mOrthographicSize = height;
    mProjectionMatrixDirty = true;
}

void Camera::SetProjection(ProjectionType type)
{
    mProjectionType = type;
    mProjectionMatrixDirty = true;
}

void Camera::SetNearPlane(float nearPlane)
{
    mNearPlane = nearPlane;
    mProjectionMatrixDirty = true;
}

void Camera::SetFarPlane(float farPlane)
{
    mFarPlane = farPlane;
    mProjectionMatrixDirty = true;
}

void Camera::SetFieldOfView(float fov)
{
    mFOV = fov;
    mProjectionMatrixDirty = true;
}

void Camera::SetOrthographicSize(float size)
{
    mOrthographicSize = size;
    mProjectionMatrixDirty = true;
}

const Matrix4& Camera::GetViewMatrix() const
{
    if (mViewMatrixDirty)
    {
        RecalculateViewMatrix();
    }
    
    return mViewMatrix;
}

const Matrix4& Camera::GetProjectionMatrix() const
{
    if (mProjectionMatrixDirty)
    {
        RecalculateProjectionMatrix();
    }
    
    return mProjectionMatrix;
}

void Camera::RecalculateViewMatrix() const
{
    mViewMatrixDirty = false;
    mViewMatrix = Matrix4::LookTo(GetWorldPosition(), ForwardVector(), UpVector());
}

void Camera::RecalculateProjectionMatrix() const
{
    mProjectionMatrixDirty = false;
    if (mProjectionType == ProjectionType::Perspective)
    {
        mProjectionMatrix = Matrix4::Perspective(mFOV, mAspectRatio, mNearPlane, mFarPlane);
    }
    else
    {
        float width = mOrthographicSize * mAspectRatio;
        float height = mOrthographicSize;
        mProjectionMatrix = Matrix4::Ortho(width, height, mNearPlane, mFarPlane);
    }
}
