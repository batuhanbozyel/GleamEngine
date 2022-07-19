#include "gpch.h"
#include "Camera.h"
#include "RendererContext.h"
#include "Core/Events/WindowEvent.h"

using namespace Gleam;

Camera::Camera()
    : Camera(RendererContext::GetProperties().width,
             RendererContext::GetProperties().height)
{
    
}

Camera::Camera(float width, float height, ProjectionType type)
    : mProjectionType(type)
{
    SetViewport(width, height);
    
    EventDispatcher<WindowResizeEvent>::Subscribe([&](const WindowResizeEvent& e) -> bool
    {
        SetViewport(e.GetWidth(),
                    e.GetHeight());
        
        return false;
    });
}

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

void Camera::Translate(const Vector3& translation)
{
    mViewMatrixDirty = true;
    Transform::Translate(translation);
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
