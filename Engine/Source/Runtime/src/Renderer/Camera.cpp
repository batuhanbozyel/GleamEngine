#include "gpch.h"
#include "Camera.h"
#include "Renderer.h"

using namespace Gleam;

Camera::Camera()
    : Camera(Renderer::GetDrawableSize())
{
	
}

Camera::Camera(float width, float height, ProjectionType type)
	: Camera({width, height}, type)
{

}

Camera::Camera(const Vector2& size, ProjectionType type)
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

void Camera::SetViewport(const Vector2& size)
{
	SetViewport(size.x, size.y);
}

void Camera::SetViewport(float width, float height)
{
	float aspectRatio = width / height;
	if (Math::Abs(aspectRatio - mAspectRatio) > Math::Epsilon)
	{
		mAspectRatio = aspectRatio;
		mProjectionMatrixDirty = true;
	}
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

void Camera::SetSize(float size)
{
    mSize = size;
    mProjectionMatrixDirty = true;
}

const Matrix4& Camera::GetViewMatrix()
{
    if (mViewMatrixDirty)
    {
        RecalculateViewMatrix();
    }
    
    return mViewMatrix;
}

const Matrix4& Camera::GetProjectionMatrix()
{
    if (mProjectionMatrixDirty)
    {
        RecalculateProjectionMatrix();
    }
    
    return mProjectionMatrix;
}

void Camera::RecalculateViewMatrix()
{
    mViewMatrixDirty = false;
    mViewMatrix = Matrix4::LookTo(GetWorldPosition(), ForwardVector(), UpVector());
}

void Camera::RecalculateProjectionMatrix()
{
    mProjectionMatrixDirty = false;
    if (mProjectionType == ProjectionType::Perspective)
    {
        mProjectionMatrix = Matrix4::Perspective(mFOV, mAspectRatio, mNearPlane, mFarPlane);
    }
    else
    {
        float width = mSize * mAspectRatio;
        float height = mSize;
        mProjectionMatrix = Matrix4::Ortho(width, height, mNearPlane, mFarPlane);
    }
}
