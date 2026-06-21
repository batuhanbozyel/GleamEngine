#pragma once
#include "Vector3.h"
#include "Vector4.h"

namespace Gleam {

GSTRUCT(Plane, "2D9F4C6A-8B13-4E57-A2D0-6F1C9B3E8A74", Serializable)
{
	GFIELD("5E1A7B92-3C44-4D8F-9A26-0B7E5C1D2F38", Serializable)
	Float3 normal{};

	GFIELD("9C3E6F18-7A52-4B0D-8E14-2D5A6C9B3F70", Serializable)
	float distance = 0.0f;

	constexpr Plane() = default;
	constexpr Plane(Plane&&) noexcept = default;
	constexpr Plane(const Plane&) = default;
	FORCE_INLINE constexpr Plane& operator=(Plane&&) noexcept = default;
	FORCE_INLINE constexpr Plane& operator=(const Plane&) = default;

	constexpr Plane(const Float3& normal, float distance)
		: normal(normal), distance(distance)
	{

	}

	// Construct from the coefficients (a, b, c, d) of the plane equation a*x + b*y + c*z + d = 0.
	constexpr explicit Plane(const Float4& coefficients)
		: normal(coefficients.x, coefficients.y, coefficients.z), distance(coefficients.w)
	{

	}
};

namespace Math {

// Returns the plane with a unit-length normal (and the offset scaled to match).
NO_DISCARD FORCE_INLINE Plane Normalize(const Plane& plane)
{
	float length = Length(plane.normal);
	return Plane{ plane.normal / length, plane.distance / length };
}

} // namespace Math

} // namespace Gleam
