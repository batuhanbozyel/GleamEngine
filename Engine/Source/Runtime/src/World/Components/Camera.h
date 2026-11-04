#pragma once
#include "Core/Attributes.h"
#include "Math/Rect.h"
#include "Math/Float4x4.h"
#include "Assets/AssetReference.h"
#include "Renderer/RenderPipelineDescriptor.h"
#include "Renderer/TextureDescriptor.h"

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
	GFIELD("5A091DDA-A9E0-4832-B86E-22F5D5C0F07D", Serializable, PrettyName("Tonemapping"))
	Tonemapping tonemapping = Tonemapping::ACES;
};

GENUM(ProjectionType, "8A1A6FA3-4FD8-4FEB-9A60-0944996B5ABF", Serializable)
{
	GITEM(Ortho, "D3640EBB-A591-461A-98AA-69700374D9C4"),
	GITEM(Perspective, "A55A66DB-9000-41ED-866D-6019900F4D2D")
};

GENUM(FieldOfViewAxis, "B078776C-EFC9-487E-8269-861B48ADC324", Serializable)
{
	GITEM(Vertical, "82876102-369D-4FAF-844D-8E36AED1BF9C"),
	GITEM(Horizontal, "1B07DB73-EF44-46E0-B859-B165346B782F")
};

GSTRUCT(Camera, "B37BA095-FFAB-49FD-ACA9-392835498015", EntityComponent, Serializable)
{
	GFIELD("A8FCFE99-7831-4104-AE5A-C344419D7A4D", Serializable, PrettyName("Field of View"))
	float fov = 60.0f;

	GFIELD("F25458F5-84F5-48D3-8E79-2531699A4765", Serializable, PrettyName("Field of View Axis"))
	FieldOfViewAxis fovAxis = FieldOfViewAxis::Vertical;

	// Half of the view extent along fovAxis, in world units
	GFIELD("1B6761D8-C039-44B0-9D78-A26DDA915D8B", Serializable, PrettyName("Orthographic Size"))
	float orthographicSize = 5.0f;

	// Normalized to the output texture, offset from the top-left corner
	GFIELD("CEB4B7B9-CBE2-4C11-AE43-268DEF3EFD42", Serializable, PrettyName("Viewport Rect"))
	Rect viewportRect;

	GFIELD("B2F2D34A-5485-4847-B27D-6BF0E3F54120", Serializable, PrettyName("Near Plane"))
	float nearPlane = 0.1f;

	GFIELD("6FC824A3-0746-433F-92BD-82767B80D3B6", Serializable, PrettyName("Far Plane"))
	float farPlane = 1000.0f;

	GFIELD("A3E08528-FB45-43C8-8D62-04584374D9C2", Serializable, PrettyName("Projection Type"))
	ProjectionType projectionType = ProjectionType::Perspective;

	GFIELD("881B9D1F-AB00-4B96-A0FB-31480CDCE38E", Serializable, PrettyName("Color Grading"))
	ColorGradingSettings colorGrading = {};

	GFIELD("B6A7FA99-185C-4A19-A67B-266E275098A9", Serializable, PrettyName("Render Pipeline"), AssetType(TypeOf<RenderPipelineDescriptor>))
	AssetReference renderPipeline;

	GFIELD("899C9F4C-6499-4A55-9234-836A9B6A0E99", Serializable, PrettyName("Target Texture"), AssetType(TypeOf<RenderTextureDescriptor>))
	AssetReference targetTexture;

	GFIELD("924C1CFA-9216-4E65-BA5D-0F3EDF5A6C67", Serializable, PrettyName("Priority"))
	int32_t priority = 0;

	Float4x4 GetProjectionMatrix(float aspectRatio) const;
};

} // namespace Gleam
