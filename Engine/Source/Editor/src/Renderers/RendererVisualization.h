//
//  RendererVisualization.h
//  Editor
//

#pragma once
#include "Core/Attributes.h"

#include "Core/Macro.h"
#include "Container/EnumFlag.h"

namespace Gleam {

GENUM(RendererVisualizationFlag, "9BB88FB4-24C7-4F0D-8EE9-D6FCCD84BE46", Serializable, PrettyName("Renderer Visualization")) : uint32_t
{
	GITEM(CameraFrustums, "53CF5C5C-FDEE-4B2D-85ED-97698361160B", PrettyName("Camera Frustums")) = BIT(0),
	GITEM(MeshBounds, "07EAFFA6-A0ED-40C9-9D72-F5832592BC57", PrettyName("Mesh Bounds")) = BIT(1)
};

GSTRUCT(RendererVisualizationSettings, "779811F9-BC78-4E69-95BF-489760A2AE14", Serializable, PrettyName("Renderer Visualization"))
{
	GFIELD("1A5BE652-91B1-4EAC-94CD-9E4C2A5BF9D0", Serializable, PrettyName("Draw"))
	EnumFlag<RendererVisualizationFlag> flags;
};

} // namespace Gleam
