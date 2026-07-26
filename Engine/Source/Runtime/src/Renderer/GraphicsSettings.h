#pragma once
#include "Renderers/AmbientOcclusionRenderer.h"
#include "Renderers/RayTracedReflectionRenderer.h"

namespace Gleam {

GSTRUCT(GraphicsSettings, "C7716B17-CC75-49FF-8893-6CC0C2B45426", Serializable, PrettyName("Graphics Settings"))
{
	GFIELD("0719AC79-EECD-4CF0-9EB8-192E999849B3", Serializable, PrettyName("Ambient Occlusion"))
	AmbientOcclusionSettings ambientOcclusion = {};

	GFIELD("E72F3CCB-C384-4CA8-A38F-43D21B577056", Serializable, PrettyName("Ray Traced Reflections"))
	RayTracedReflectionSettings rayTracedReflections = {};
};

} // namespace Gleam
