#include "gpch.h"
#include "Camera.h"

using namespace Gleam;

void Camera::SetViewport(float width, float height)
{
    mAspectRatio = width / height;
    mProjectionMatrixDirty = true;
}

void Camera::SetProjection(ProjectionType type)
{
    mProjectionType = type;
    mProjectionMatrixDirty = true;
}

void Camera::Update()
{
    if (mProjectionMatrixDirty)
    {
        RecalculateProjectionMatrix();
    }
    RecalculateViewMatrix();
}

const Matrix4& Camera::GetViewMatrix() const
{
    return mViewMatrix;
}

const Matrix4& Camera::GetProjectionMatrix() const
{
    return mProjectionMatrix;
}

void Camera::RecalculateViewMatrix()
{
    mViewMatrix = Matrix4::LookAt(GetWorldPosition(), GetWorldPosition() + ForwardVector(), UpVector());
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
        float right = mSize * mAspectRatio;
        float left = -right;
        float top = mSize;
        float bottom = -top;
        mProjectionMatrix = Matrix4::Ortho(left, right, bottom, top, mNearPlane, mFarPlane);
    }
}
