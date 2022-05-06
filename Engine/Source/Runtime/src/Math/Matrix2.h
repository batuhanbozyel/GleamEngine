#pragma once

namespace Gleam {
	
struct Matrix2
{
	union
	{
		TArray<float, 4> value;
		TArray<Vector2, 2> row;
	};

    static const Matrix2 zero;
    static const Matrix2 identity;

	Matrix2() = default;
	constexpr Matrix2(Matrix2&&) = default;
	constexpr Matrix2(const Matrix2&) = default;
	inline constexpr Matrix2& operator=(Matrix2&&) = default;
	inline constexpr Matrix2& operator=(const Matrix2&) = default;

	constexpr Matrix2(float m00, float m01,
					  float m10, float m11)
		: value{m00, m01,
				m10, m11}
	{

	}
	constexpr Matrix2(const Vector2& row0, const Vector2& row1)
		: row{ row0, row1 }
	{

	}
	constexpr Matrix2(const TArray<float, 4>& list)
		: value(list)
	{

	}
	constexpr Matrix2(const TArray<Vector2, 2>& list)
		: row(list)
	{

	}

	MATH_INLINE constexpr const Vector2& operator[](size_t i) const
	{
		return row[i];
	}

};

} // namespace Gleam
