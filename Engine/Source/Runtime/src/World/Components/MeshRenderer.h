#pragma once
#include "Assets/AssetReference.h"

namespace Gleam {

GSTRUCT(MeshRenderer, "71B269C7-DCF6-4E00-A914-62CE71321893", EntityComponent, Serializable)
{
	GFIELD("74C44005-75FD-4E1E-8DD9-A63D54A6E486", Serializable, PrettyName("Mesh"))
	AssetReference mesh;

	GFIELD("7ABC9B5E-CA6B-4E72-B14D-4546C4472C15", Serializable, PrettyName("Materials"))
	TArray<AssetReference> materials;
};

} // namespace Gleam