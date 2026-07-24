#pragma once
#include <Reflection/Macro.h>

namespace Gleam {

GENUM(ReflectionProbeResolution, "07F67161-8957-48BB-94DA-882D9861F06B", Serializable, PrettyName("Reflection Probe Resolution"))
{
	GITEM(ReflectionProbeResolution128, "DA18E0F3-CC37-4CDC-BC27-868291B03C67", PrettyName("128")) = 128,
	GITEM(ReflectionProbeResolution256, "9954B9A3-8911-417F-A1B5-4FB3051AF2A2", PrettyName("256")) = 256,
	GITEM(ReflectionProbeResolution512, "E8A0DE3A-441D-4190-8A68-F9405ADD219E", PrettyName("512")) = 512,
	GITEM(ReflectionProbeResolution1024, "4E6BA709-0D43-47E3-BC70-596AB090E207", PrettyName("1024")) = 1024,
	GITEM(ReflectionProbeResolution2048, "0ACE9D0F-20AF-4F95-8DF2-FFC4944E202C", PrettyName("2048")) = 2048
};

GSTRUCT(ReflectionProbe, "96838A36-D51D-4A58-93E6-F76D7624BCA2", EntityComponent, Serializable, PrettyName("Reflection Probe"))
{
	GFIELD("6D690D24-FA95-4297-A748-858132BEC99A", Serializable, PrettyName("Resolution"))
	ReflectionProbeResolution resolution = ReflectionProbeResolution::ReflectionProbeResolution256;
};

} // namespace Gleam
