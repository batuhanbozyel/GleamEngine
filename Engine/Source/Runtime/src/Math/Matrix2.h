#pragma once

namespace Gleam {
	
struct Matrix2
{
	union
	{
		TArray<float, 4> m;
		TArray<Vector2, 2> row;
	};

    static const Matrix2 zero;
    static const Matrix2 identity;

	Matrix2() = default;
	constexpr Matrix2(Matrix2&&) = default;
	constexpr Matrix2(const Matrix2&) = default;
	FORCE_INLINE constexpr Matrix2& operator=(Matrix2&&) = default;
    FORCE_INLINE constexpr Matrix2& operator=(const Matrix2&) = default;

	constexpr Matrix2(float m00, float m01,
					  float m10, float m11)
		: m{m00, m01,
            m10, m11}
	{

	}
	constexpr Matrix2(const Vector2& row0, const Vector2& row1)
		: row{ row0, row1 }
	{

	}
	constexpr Matrix2(const TArray<float, 4>& m)
		: m(m)
	{

	}
	constexpr Matrix2(const TArray<Vector2, 2>& row)
		: row(row)
	{

	}

	NO_DISCARD FORCE_INLINE constexpr const Vector2& operator[](size_t i) const
	{
		return row[i];
	}
    
    NO_DISCARD FORCE_INLINE constexpr Matrix2 operator*(const Matrix2& rhs) const
    {
        return Matrix2
        {
            rhs.m[0] * m[0] + rhs.m[1] * m[2],
            rhs.m[0] * m[1] + rhs.m[1] * m[3],

            rhs.m[2] * m[0] + rhs.m[3] * m[2],
            rhs.m[2] * m[1] + rhs.m[3] * m[3]
        };
    }
    
    FORCE_INLINE constexpr Matrix2& operator*=(const Matrix2& rhs)
    {
        return *this = *this * rhs;
    }

};

} // namespace Gleam
