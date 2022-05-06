#pragma once

namespace Gleam {
	
struct Matrix3
{
	union
	{
		TArray<float, 9> value;
		TArray<Vector3, 3> row;
	};
    
    static const Matrix3 zero;
    static const Matrix3 identity;

	Matrix3() = default;
	constexpr Matrix3(Matrix3&&) = default;
	constexpr Matrix3(const Matrix3&) = default;
	inline constexpr Matrix3& operator=(Matrix3&&) = default;
	inline constexpr Matrix3& operator=(const Matrix3&) = default;

	constexpr Matrix3(float m00, float m01, float m02,
					  float m10, float m11, float m12,
					  float m20, float m21, float m22)
		: value{m00, m01, m02,
				m10, m11, m12,
				m20, m21, m22}
	{

	}
	constexpr Matrix3(const Vector3& row0, const Vector3& row1, const Vector3& row2)
		: row{ row0, row1, row2 }
	{

	}
	constexpr Matrix3(const TArray<float, 9>& list)
		: value(list)
	{

	}
	constexpr Matrix3(const TArray<Vector3, 3>& list)
		: row(list)
	{

	}

	MATH_INLINE constexpr const Vector3& operator[](size_t i) const
	{
		return row[i];
	}

};

} // namespace Gleam
