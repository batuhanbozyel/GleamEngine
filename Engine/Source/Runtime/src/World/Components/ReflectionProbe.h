#pragma once
#include <Reflection/Macro.h>
#include <cstdint>

namespace Gleam {

GSTRUCT(ReflectionProbe, "96838A36-D51D-4A58-93E6-F76D7624BCA2", Serializable)
{
	GFIELD("6D690D24-FA95-4297-A748-858132BEC99A", Serializable)
	uint32_t size = 512;
};

} // namespace Gleam
