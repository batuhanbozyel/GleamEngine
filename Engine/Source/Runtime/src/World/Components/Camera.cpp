#include "gpch.h"
#include "Camera.h"

namespace Gleam {

Float4x4 Camera::GetProjectionMatrix(float aspectRatio) const
{
	if (projectionType == ProjectionType::Perspective)
	{
		float verticalFov = Math::Deg2Rad(fov);
		if (fovAxis == FieldOfViewAxis::Horizontal)
		{
			verticalFov = 2.0f * Math::Atan(Math::Tan(verticalFov * 0.5f) / aspectRatio);
		}
		return Float4x4::Perspective(verticalFov, aspectRatio, nearPlane, farPlane);
	}

	const float height = fovAxis == FieldOfViewAxis::Vertical ? 2.0f * orthographicSize : 2.0f * orthographicSize / aspectRatio;
	return Float4x4::Ortho(height * aspectRatio, height, nearPlane, farPlane);
}

} // namespace Gleam
