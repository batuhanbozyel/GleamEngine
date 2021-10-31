#pragma once

namespace Gleam {

namespace Math {

class Transform
{
public:

	constexpr inline void Translate(const Vector3& translation)
	{
		m_Position = translation;
	}

	constexpr inline void Rotate(const Quaternion& quat)
	{
		
	}

	constexpr inline void Rotate(const Vector3& axis, float angle)
	{

	}

	constexpr inline void Scale(const Vector3& scale)
	{
		m_Scale = scale;
	}

	constexpr inline void UniformScale(float scale)
	{
		m_Scale = Vector3(scale);
	}

private:

	Vector3 m_Position{ 0.0f };
	Vector3 m_Rotation{ 0.0f };
	Vector3 m_Scale{ 1.0f };
};

}}