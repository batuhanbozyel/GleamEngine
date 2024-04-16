#pragma once

namespace Gleam {

struct Float3x3
{
    union
    {
        TArray<float, 9> m;
        TArray<Float3, 3> row{};
    };

    static const Float3x3 zero;
    static const Float3x3 identity;

    constexpr Float3x3() = default;
    constexpr Float3x3(Float3x3&&) noexcept = default;
    constexpr Float3x3(const Float3x3&) = default;
    FORCE_INLINE constexpr Float3x3& operator=(Float3x3&&) noexcept = default;
    FORCE_INLINE constexpr Float3x3& operator=(const Float3x3&) = default;

    constexpr Float3x3(float m00, float m01, float m02,
                      float m10, float m11, float m12,
                      float m20, float m21, float m22)
        : m{m00, m01, m02,
            m10, m11, m12,
            m20, m21, m22}
    {

    }
    constexpr Float3x3(const Float3& row0, const Float3& row1, const Float3& row2)
        : row{ row0, row1, row2 }
    {

    }
    constexpr Float3x3(const TArray<float, 9>& m)
        : m(m)
    {

    }
    constexpr Float3x3(const TArray<Float3, 3>& row)
        : row(row)
    {

    }

    NO_DISCARD FORCE_INLINE constexpr const Float3& operator[](size_t i) const
    {
        return row[i];
    }

    NO_DISCARD FORCE_INLINE constexpr Float3 operator*(const Float3& vec) const
    {
        return Float3
        {
            vec.x * m[0] + vec.y * m[3] + vec.z * m[6],
            vec.x * m[1] + vec.y * m[4] + vec.z * m[7],
            vec.x * m[2] + vec.y * m[5] + vec.z * m[8]
        };
    }

    NO_DISCARD FORCE_INLINE constexpr Float3x3 operator*(const Float3x3& rhs) const
    {
        return Float3x3
        {
            rhs.m[0] * m[0] + rhs.m[1] * m[3] + rhs.m[2] * m[6],
            rhs.m[0] * m[1] + rhs.m[1] * m[4] + rhs.m[2] * m[7],
            rhs.m[0] * m[2] + rhs.m[1] * m[5] + rhs.m[2] * m[8],

            rhs.m[3] * m[0] + rhs.m[4] * m[3] + rhs.m[5] * m[6],
            rhs.m[3] * m[1] + rhs.m[4] * m[4] + rhs.m[5] * m[7],
            rhs.m[3] * m[2] + rhs.m[4] * m[5] + rhs.m[5] * m[8],

            rhs.m[6] * m[0] + rhs.m[7] * m[3] + rhs.m[8] * m[6],
            rhs.m[6] * m[1] + rhs.m[7] * m[4] + rhs.m[8] * m[7],
            rhs.m[6] * m[2] + rhs.m[7] * m[5] + rhs.m[8] * m[8]
        };
    }

    FORCE_INLINE constexpr Float3x3& operator*=(const Float3x3& rhs)
    {
        return *this = *this * rhs;
    }

};

} // namespace Gleam
