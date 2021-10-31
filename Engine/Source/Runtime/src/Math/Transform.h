#pragma once

namespace Gleam {

namespace Math {

class Transform
{
public:

	MATH_INLINE constexpr void Translate(const Vector3& translation)
	{
		m_Position = translation;
	}

	MATH_INLINE constexpr void Rotate(const Quaternion& quat)
	{
		
	}

	MATH_INLINE constexpr void Rotate(const Vector3& axis, float angle)
	{

	}

	MATH_INLINE constexpr void Scale(const Vector3& scale)
	{
		m_Scale = scale;
	}

	MATH_INLINE constexpr void UniformScale(float scale)
	{
		m_Scale = Vector3(scale);
	}

private:

	Vector3 m_Position{ 0.0f };
	Vector3 m_Rotation{ 0.0f };
	Vector3 m_Scale{ 1.0f };
};

}}