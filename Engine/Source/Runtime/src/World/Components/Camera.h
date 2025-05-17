#pragma once
#include "Reflection/Macro.h"

namespace Gleam {

GENUM(Tonemapping, "6B993432-E807-444D-AB3D-8B6F6BD8F84D", Serializable)
{
	GITEM(None, "68F038B5-1C17-4F17-BCE6-FEF17DA827E0"),
	GITEM(ACES, "D5230C44-E382-42E9-ACE6-29546B6B853A"),
	GITEM(Neutral, "0CE6518D-9656-41DC-A63A-191FCB15E3E8"),
	GITEM(Filmic, "715E0B8F-25D1-4D4F-8BC2-C711A4239909"),
	GITEM(AgX, "E3A90B5F-CAF3-41CF-8D43-76FFCE181DF7")
};

GSTRUCT(ColorGradingSettings, "CD1CEEDD-2481-4000-B165-BCC6A1953E00", Serializable)
{
	GFIELD("5A091DDA-A9E0-4832-B86E-22F5D5C0F07D", Serializable)
	Tonemapping tonemapping = Tonemapping::ACES;
};

GENUM(ProjectionType, "8A1A6FA3-4FD8-4FEB-9A60-0944996B5ABF", Serializable)
{
	GITEM(Ortho, "D3640EBB-A591-461A-98AA-69700374D9C4"),
	GITEM(Perspective, "A55A66DB-9000-41ED-866D-6019900F4D2D")
};

GSTRUCT(Camera, "CD1CEEDD-2481-4000-B165-BCC6A1953E00", EntityComponent, Serializable)
{
	// Perspective projection properties
	GFIELD("A8FCFE99-7831-4104-AE5A-C344419D7A4D", Serializable)
	float fov = 60.0f;

	// Orthographic projection properties
	GFIELD("1B6761D8-C039-44B0-9D78-A26DDA915D8B", Serializable)
	float orthographicSize = 5.0f;

	// Common properties
	GFIELD("FB484D81-6A77-4D1B-BBE3-63C7491D844B", Serializable)
	float aspectRatio = 1.0f;

	GFIELD("B2F2D34A-5485-4847-B27D-6BF0E3F54120", Serializable)
	float nearPlane = 0.1f;

	GFIELD("6FC824A3-0746-433F-92BD-82767B80D3B6", Serializable)
	float farPlane = 1000.0f;

	GFIELD("A3E08528-FB45-43C8-8D62-04584374D9C2", Serializable)
	ProjectionType projectionType = ProjectionType::Perspective;

	// Post-process settings
	GFIELD("881B9D1F-AB00-4B96-A0FB-31480CDCE38E", Serializable)
	ColorGradingSettings colorGrading = {};
    
	Camera() = default;

    Camera(const Size& size, ProjectionType type = ProjectionType::Perspective);

    Camera(float width, float height, ProjectionType type = ProjectionType::Perspective);
    
    void SetViewport(const Size& size);

    void SetViewport(float width, float height);

	Size GetViewport() const;
};

} // namespace Gleam
