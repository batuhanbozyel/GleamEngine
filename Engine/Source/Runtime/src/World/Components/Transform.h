#pragma once
#include "Math/Quaternion.h"
#include "Math/Float4x4.h"

namespace Gleam {

GSTRUCT(Transform, "D534ACED-5A81-4183-BD5F-A7F61A8F47E7", Serializable)
{
	GFIELD("4A96F8C0-FB46-42EF-8791-2E7673A10210", Serializable)
	Float3 position = Float3::zero;

	GFIELD("B1D05665-F562-4239-B516-6CE201DFB128", Serializable)
	Quaternion rotation = Quaternion::identity;

	GFIELD("4C236111-7E2A-4BD4-BCA8-9F58DF41480A", Serializable)
	float scale = 1.0f;

	operator Float4x4() const
	{
		return Float4x4::TRS(position, rotation, scale);
	}

	Transform operator*(const Transform& rhs) const
	{
		return Transform
		{
			.position = position + (rotation * (rhs.position * scale)),
			.rotation = rotation * rhs.rotation,
			.scale = scale * rhs.scale
		};
	}
};

} // namespace Gleam