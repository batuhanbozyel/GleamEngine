#pragma once
#include "Container/Array.h"
#include "Container/BinaryBuffer.h"

#include <Reflection/Reflection.h>
#ifndef __GLEAM_REFLECTION__
#include <Runtime.Reflection.generated.h>
#endif

namespace Gleam {

GSTRUCT(AssetDataTable, "91170B46-96F7-48CE-9B16-9946705CF8C6", Serializable)
{
	GFIELD("27E3C612-F302-4E42-96D8-0447D272AFAD", Serializable)
	TArray<BufferRange> blobs;
};

} // namespace Gleam
