#pragma once
#include "Core/GUID.h"

namespace Gleam {

struct EntityReference
{
	Guid guid = Guid::InvalidGuid();

	operator bool() const
	{
		return guid != Guid::InvalidGuid();
	}
};

} // namespace Gleam

GLEAM_TYPE(Gleam::EntityReference, Guid("AED872F7-F974-492E-AF0B-93E7FF6DD2E7"))
    GLEAM_FIELD(guid, Serializable())
GLEAM_END