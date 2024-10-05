#pragma once
#include "Assets/AssetReference.h"

namespace Gleam {

struct MeshRenderer
{
	AssetReference mesh;
	TArray<AssetReference> materials;
};

} // namespace Gleam

GLEAM_TYPE(Gleam::MeshRenderer, Guid("71B269C7-DCF6-4E00-A914-62CE71321893"), EntityComponent())
	GLEAM_FIELD(mesh, Serializable())
	GLEAM_FIELD(materials, Serializable())
GLEAM_END