#include "gpch.h"
#include "Camera.h"

using namespace Gleam;

Camera::Camera(float width, float height, ProjectionType type)
	: Camera({width, height}, type)
{

}

Camera::Camera(const Size& size, ProjectionType type)
	: projectionType(type)
{
	SetViewport(size);
}

void Camera::SetViewport(const Size& size)
{
	SetViewport(size.width, size.height);
}

void Camera::SetViewport(float width, float height)
{
    aspectRatio = width / height;
    orthographicSize = height;
}
