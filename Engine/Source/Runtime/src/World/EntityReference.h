#pragma once
#include "Core/GUID.h"

namespace Gleam {

GSTRUCT(EntityReference, "AED872F7-F974-492E-AF0B-93E7FF6DD2E7", Serializable)
{
	GFIELD("B4C31D9E-7A57-4F2A-BAE5-D5F76E456C3D", Serializable)
	Guid guid = Guid::InvalidGuid();

	operator bool() const
	{
		return guid != Guid::InvalidGuid();
	}
};

} // namespace Gleam
