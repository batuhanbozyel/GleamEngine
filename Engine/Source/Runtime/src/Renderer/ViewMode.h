#pragma once

#if defined(__cplusplus)
#include <Reflection/Macro.h>
#else
#define GENUM(Name, GuidStr, ...) enum class Name
#define GITEM(Name, GuidStr, ...) Name
#endif

namespace Gleam {

// TODO: Move ViewMode to the Editor project — it is an editor-only debug visualization concept.
// It lives in Runtime for now only because reflection is single-module/Runtime-only (one global
// IDatabase + index-based generated accessors, like RenderPath in Renderer/RenderSystem.h). Once
// the reflection system is refactored to replace the binary database with baked headers
// (hash-based, multi-module), relocate this enum to the Editor alongside ViewModeRenderer.
GENUM(ViewMode, "984C1E2A-2798-4A6A-97A7-517BCE9E77D0", PrettyName("View Mode"))
{
	GITEM(Lit, "1B47DE32-C98D-4B62-A568-BB0FB57CD2E0", PrettyName("Lit")),
	GITEM(ShadingNormal, "A52A5365-AA41-4A63-A225-0C0A9CBA4895", PrettyName("Shading Normal")),
	GITEM(GeometryNormal, "BCE73F3E-390A-462A-B50B-24EAB3BBA358", PrettyName("Geometry Normal")),
	GITEM(Depth, "CAB78D5F-5494-49AA-AD5A-299384016075", PrettyName("Depth")),
	GITEM(Roughness, "45D58C71-9BFB-45E4-80C1-4A82961F186C", PrettyName("Roughness")),
	GITEM(MotionVectors, "FF60C785-5918-47DD-AC7C-06E39DF27A0C", PrettyName("Motion Vectors")),
	GITEM(ShadowMask, "42865A07-40E0-460C-862B-BC717ED692E5", PrettyName("Shadow Mask")),
	GITEM(AmbientOcclusion, "FB2F9437-1E38-4999-B584-A9F3C9B512CB", PrettyName("Ambient Occlusion")),
	GITEM(MeshletVisualization, "C7E4A8D2-3F19-4B6A-8E2D-5A9C1B0F7E34", PrettyName("Meshlet Visualization")),
	GITEM(VisibilityIDs, "838BE469-77E5-4874-B145-9D333EB400E1", PrettyName("Visibility IDs")),
	GITEM(BatchIDs, "808B9D44-FE18-439D-9730-25CEBFA3870D", PrettyName("Batch IDs"))
};

} // namespace Gleam

#if !defined(__cplusplus)
#undef GENUM
#undef GITEM
#endif
