#pragma once
#include "Core/GUID.h"
#include "Container/Array.h"

namespace Gleam {

GSTRUCT(RenderPipelineDescriptor, "F81D0A1B-0DAB-4DB0-9338-4CAD8D9F19C3", Serializable, PrettyName("Render Pipeline"))
{
	GFIELD("05949F9B-0DE0-4BE9-B4A7-2328B4CB5EE6", Serializable)
	TArray<Guid> renderers;
};

} // namespace Gleam
