#pragma once
#include "Renderer/RenderSurface.h"

namespace Gleam {

enum class Tonemapping
{
	None,
	ACES,
	Neutral,
	Filmic,
	AgX
};

struct ColorGradingSettings
{
	Tonemapping tonemapping = Tonemapping::ACES;
};

enum class ProjectionType
{
	Ortho,
	Perspective
};

struct Camera
{
	// Perspective projection properties
	float fov = 60.0f;

	// Orthographic projection properties
	float orthographicSize = 5.0f;

	// Common properties
	float aspectRatio = 1.0f;
	float nearPlane = 0.1f;
	float farPlane = 1000.0f;
	ProjectionType projectionType = ProjectionType::Perspective;

	// Post-process settings
	ColorGradingSettings colorGrading = {};
    
	Camera() = default;

    Camera(const Size& size, ProjectionType type = ProjectionType::Perspective);

    Camera(float width, float height, ProjectionType type = ProjectionType::Perspective);
    
    void SetViewport(const Size& size);

    void SetViewport(float width, float height);

	Size GetViewport() const;
};

} // namespace Gleam

GLEAM_ENUM(Gleam::Tonemapping, Guid("6B993432-E807-444D-AB3D-8B6F6BD8F84D"))
GLEAM_TYPE(Gleam::ColorGradingSettings, Guid("CD1CEEDD-2481-4000-B165-BCC6A1953E00"), EntityComponent())
	GLEAM_FIELD(tonemapping, Serializable())
GLEAM_END


GLEAM_ENUM(Gleam::ProjectionType, Guid("8A1A6FA3-4FD8-4FEB-9A60-0944996B5ABF"))

GLEAM_TYPE(Gleam::Camera, Guid("CD1CEEDD-2481-4000-B165-BCC6A1953E00"), EntityComponent())
	GLEAM_FIELD(fov, Serializable())
	GLEAM_FIELD(orthographicSize, Serializable())
	GLEAM_FIELD(aspectRatio, Serializable())
	GLEAM_FIELD(nearPlane, Serializable())
	GLEAM_FIELD(farPlane, Serializable())
	GLEAM_FIELD(projectionType, Serializable())
GLEAM_END
