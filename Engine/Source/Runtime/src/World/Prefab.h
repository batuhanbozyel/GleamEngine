#pragma once
#include "Assets/AssetReference.h"

namespace Gleam {

struct Prefab
{
    AssetReference reference;
	//AssetReference parent; // TODO: how to handle nested prefabs?
};

} // namespace Gleam

GLEAM_TYPE(Gleam::Prefab, Guid("CAFCF979-D525-48D5-81CD-76731218F4DA"))
    GLEAM_FIELD(reference, Serializable())
GLEAM_END