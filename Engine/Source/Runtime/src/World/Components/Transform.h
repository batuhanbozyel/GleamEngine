#pragma once

namespace Gleam {

struct Transform
{
	Float3 position = Float3(0.0f, 0.0f, 0.0f);
	Quaternion rotation = Quaternion::identity;
	float scale = 1.0f;

	operator Float4x4() const
	{
		return Float4x4::TRS(position, rotation, scale);
	}

	Transform operator*(const Transform& rhs) const
	{
		auto nonUniformScale = Math::Inverse(rhs.rotation) * ((rhs.rotation * rhs.scale) * scale);
		return Transform
		{
			.position = position + (rotation * (rhs.position * scale)),
			.rotation = rotation * rhs.rotation,
			.scale = (nonUniformScale.x + nonUniformScale.y + nonUniformScale.z) / 3.0f
		};
	}
};

} // namespace Gleam

GLEAM_TYPE(Gleam::Transform, Guid("D534ACED-5A81-4183-BD5F-A7F61A8F47E7"))
	GLEAM_FIELD(position, Serializable())
	GLEAM_FIELD(rotation, Serializable())
	GLEAM_FIELD(scale, Serializable())
GLEAM_END