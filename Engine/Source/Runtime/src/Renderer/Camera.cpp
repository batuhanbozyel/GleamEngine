#include "gpch.h"
#include "Camera.h"
#include "Renderer.h"

using namespace Gleam;

Camera::Camera()
    : Camera(Renderer::GetDrawableSize())
{
	EventDispatcher<RendererDrawableResizeEvent>::Subscribe([&](const RendererDrawableResizeEvent& e) -> bool
	{
		SetViewport(e.GetWidth(), e.GetHeight());
		return false;
	});
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

void Camera::SetViewport(const Vector2& size)
{
	SetViewport(size.x, size.y);
}

void Camera::SetViewport(float width, float height)
{
	float aspectRatio = width / height;
	if (Math::Abs(aspectRatio - mAspectRatio) > Math::Epsilon)
	{
        mSize = height;
		mAspectRatio = aspectRatio;
		mProjectionMatrixDirty = true;
	}
}

void Camera::SetProjection(ProjectionType type)
{
    mProjectionType = type;
    mProjectionMatrixDirty = true;
}

void Camera::Translate(const Vector3& translation)
{
    mViewMatrixDirty = true;
    Transform::Translate(translation);
}

void Camera::Rotate(const Quaternion& quat)
{
    mViewMatrixDirty = true;
    Transform::Rotate(quat);
}

void Camera::Rotate(const Vector3& eulers)
{
    Rotate(Quaternion(eulers));
}

void Camera::Rotate(float xAngle, float yAngle, float zAngle)
{
    Rotate(Vector3{xAngle, yAngle, zAngle});
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
