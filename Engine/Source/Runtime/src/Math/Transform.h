#pragma once

namespace Gleam {

struct Transform
{
	Vector3 position{ 0.0f };
	Vector3 rotation{ 0.0f };
	Vector3 scale{ 1.0f };

	MATH_INLINE constexpr void Translate(const Vector3& translation)
	{
		// TODO:
	}

	MATH_INLINE constexpr void Rotate(const Quaternion& quat)
	{
		// TODO:
	}

	MATH_INLINE constexpr void Rotate(const Vector3& axis, float angle)
	{
		// TODO:
	}

	MATH_INLINE constexpr void Scale(const Vector3& scale)
	{
		// TODO:
	}

	MATH_INLINE constexpr void UniformScale(float scale)
	{
		// TODO:
	}

};

}