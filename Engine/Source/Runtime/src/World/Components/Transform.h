#pragma once

namespace Gleam {

struct Transform
{
	Float3 position = Float3(0.0f, 0.0f, 0.0f);
	Quaternion rotation = Quaternion::identity;
	Float3 scale = Float3(1.0f, 1.0f, 1.0f);

	mutable Float4x4 matrix = Float4x4::identity;

	operator Float4x4() const
	{
		return matrix;
	}
};

} // namespace Gleam

GLEAM_TYPE(Gleam::Transform, Guid("D534ACED-5A81-4183-BD5F-A7F61A8F47E7"))
	GLEAM_FIELD(position, Serializable())
	GLEAM_FIELD(rotation, Serializable())
	GLEAM_FIELD(scale, Serializable())
GLEAM_END