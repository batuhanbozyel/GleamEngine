#pragma once

namespace Gleam {
	
struct Float2x2
{
    union
    {
        TArray<float, 4> m;
        TArray<Float2, 2> row{};
    };

    static const Float2x2 zero;
    static const Float2x2 identity;

    constexpr Float2x2() = default;
    constexpr Float2x2(Float2x2&&) noexcept = default;
    constexpr Float2x2(const Float2x2&) = default;
    FORCE_INLINE constexpr Float2x2& operator=(Float2x2&&) noexcept = default;
    FORCE_INLINE constexpr Float2x2& operator=(const Float2x2&) = default;

    constexpr Float2x2(float m00, float m01,
                      float m10, float m11)
        : m{m00, m01,
            m10, m11}
    {

    }
    constexpr Float2x2(const Float2& row0, const Float2& row1)
        : row{ row0, row1 }
    {

    }
    constexpr Float2x2(const TArray<float, 4>& m)
        : m(m)
    {

    }
    constexpr Float2x2(const TArray<Float2, 2>& row)
        : row(row)
    {

    }

    NO_DISCARD FORCE_INLINE constexpr const Float2& operator[](size_t i) const
    {
        return row[i];
    }

    NO_DISCARD FORCE_INLINE constexpr Float2 operator*(const Float2& vec) const
    {
        return Float2
        {
            vec.x * m[0] + vec.y * m[2],
            vec.x * m[1] + vec.y * m[3]
        };
    }

    NO_DISCARD FORCE_INLINE constexpr Float2x2 operator*(const Float2x2& rhs) const
    {
        return Float2x2
        {
            rhs.m[0] * m[0] + rhs.m[1] * m[2],
            rhs.m[0] * m[1] + rhs.m[1] * m[3],

            rhs.m[2] * m[0] + rhs.m[3] * m[2],
            rhs.m[2] * m[1] + rhs.m[3] * m[3]
        };
    }

    FORCE_INLINE constexpr Float2x2& operator*=(const Float2x2& rhs)
    {
        return *this = *this * rhs;
    }

};

} // namespace Gleam

GLEAM_TYPE(Gleam::Float2x2, Guid("8C079A6F-245E-4692-837D-0B54D6810B81"))
	GLEAM_FIELD(row, Serializable())
GLEAM_END