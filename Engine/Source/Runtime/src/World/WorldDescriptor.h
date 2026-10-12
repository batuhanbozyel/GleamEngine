#pragma once
#include "Math/Vector3.h"

namespace Gleam {

GSTRUCT(WorldDescriptor, "8F9A4006-5E9C-4C13-81FB-6F22ED723075", Serializable)
{
	GFIELD("9A2E28EF-7E09-4265-97E4-EAF03236A214", Serializable)
	Float3 gravity = Float3{ 0.0f, -9.81f, 0.0f };
};

} // namespace Gleam
