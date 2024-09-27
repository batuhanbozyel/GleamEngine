#pragma once
#include "World/EntityReference.h"

namespace Gleam {

struct Transform
{
	Float3 position = Float3(0.0f, 0.0f, 0.0f);
	Quaternion rotation = Quaternion::identity;
	Float3 scale = Float3(1.0f, 1.0f, 1.0f);

	mutable Float4x4 matrix = Float4x4::identity;
};

} // namespace Gleam