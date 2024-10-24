#pragma once

namespace Gleam {

struct Color32;

struct Color : public Float4
{
	static const Color clear;
	static const Color black;
	static const Color white;
	static const Color gray;
	static const Color red;
	static const Color green;
	static const Color blue;
	static const Color cyan;
	static const Color magenta;
	static const Color yellow;

	constexpr float MaxComponent() const
	{
		return Math::Max(r, Math::Max(g, Math::Max(b, a)));
	}

	NO_DISCARD FORCE_INLINE static constexpr Color HSVToRGB(float h, float s, float v)
	{
		return Math::Mix(Float3(1.0f), Math::Clamp(Math::Abs(Math::Fract(Float3(h + 1.0f, h + 2.0f / 3.0f, h + 1.0f / 3.0f)) * 6.0f - 3.0f) - 1.0f, 0.0f, 1.0f), s) * v;
	}

	NO_DISCARD FORCE_INLINE static constexpr Color HSVToRGB(const Color& hsv)
	{
		return Math::Mix(Float3(1.0f), Math::Clamp(Math::Abs(Math::Fract(Float3(hsv.x + 1.0f, hsv.x + 2.0f / 3.0f, hsv.x + 1.0f / 3.0f)) * 6.0f - 3.0f) - 1.0f, 0.0f, 1.0f), hsv.y) * hsv.z;
	}

	NO_DISCARD FORCE_INLINE static constexpr Color RGBToHSV(float r, float g, float b)
	{
		constexpr Float4 K = Float4(0.0f, -1.0f / 3.0f, 2.0f / 3.0f, -1.0f);

		Float4 p = Math::Mix(Float4(b, g, K.w, K.z), Float4(g, b, K.x, K.y), Math::Step(b, g));
		Float4 q = Math::Mix(Float4(p.x, p.y, p.w, r), Float4(r, p.y, p.z, p.x), Math::Step(p.x, r));
		float d = q.x - Math::Min(q.w, q.y);

		return Color(Math::Abs(q.z + (q.w - q.y) / (6.0f * d + Math::Epsilon)), d / (q.x + Math::Epsilon), q.x);
	}

	NO_DISCARD FORCE_INLINE static constexpr Color RGBToHSV(const Color& color)
	{
		constexpr Float4 K = Float4(0.0f, -1.0f / 3.0f, 2.0f / 3.0f, -1.0f);

		Float4 p = Math::Mix(Float4(color.b, color.g, K.w, K.z), Float4(color.g, color.b, K.x, K.y), Math::Step(color.b, color.g));
		Float4 q = Math::Mix(Float4(p.x, p.y, p.w, color.r), Float4(color.r, p.y, p.z, p.x), Math::Step(p.x, color.r));
		float d = q.x - Math::Min(q.w, q.y);

		return Color(Math::Abs(q.z + (q.w - q.y) / (6.0f * d + Math::Epsilon)), d / (q.x + Math::Epsilon), q.x);
	}

	// Constructors
	constexpr Color() = default;
	constexpr Color(Color&&) noexcept = default;
	constexpr Color(const Color&) = default;

	constexpr Color(std::floating_point auto v)
		: Float4(v)
	{

	}
	constexpr Color(std::floating_point auto r,
					std::floating_point auto g,
					std::floating_point auto b)
		: Float4(r, g, b, 1.0f)
	{

	}
	constexpr Color(std::floating_point auto r,
					std::floating_point auto g,
					std::floating_point auto b,
					std::floating_point auto a)
		: Float4(r, g, b, a)
	{

	}
	constexpr Color(const TArray<float, 3>& color)
		: Float4(color[0], color[1], color[2], 1.0f)
	{

	}
	constexpr Color(const TArray<float, 4>& color)
		: Float4(color)
	{

	}
	constexpr Color(const Float3& color)
		: Float4(color.r, color.g, color.b, 1.0f)
	{

	}
	constexpr Color(const Float4& color)
		: Float4(color)
	{

	}
	constexpr Color(Float4&& color) noexcept
		: Float4(std::move(color))
	{

	}

	// Operator overloads
    FORCE_INLINE constexpr Color& operator=(Color&&) noexcept = default;
    FORCE_INLINE constexpr Color& operator=(const Color&) = default;

    FORCE_INLINE constexpr Color& operator=(const Float4& color)
	{
		value = color.value;
		return *this;
	}
    FORCE_INLINE constexpr Color& operator=(Float4&& color) noexcept
	{
		value = std::move(color.value);
		return *this;
	}

	NO_DISCARD FORCE_INLINE constexpr operator Color32() const;
	FORCE_INLINE constexpr Color& operator=(const Color32&);
    
    NO_DISCARD FORCE_INLINE constexpr bool operator==(const Color& other) const
    {
        return (Math::Abs(r - other.r) < Math::Epsilon) && (Math::Abs(g - other.g) < Math::Epsilon) && (Math::Abs(b - other.b) < Math::Epsilon) && (Math::Abs(a - other.a) < Math::Epsilon);
    }
    
    NO_DISCARD FORCE_INLINE constexpr bool operator!=(const Color& other) const
    {
        return !(*this == other);
    }

};

struct Color32
{
	uint8_t r = 0, g = 0, b = 0, a = 0;

	constexpr Color32() = default;
	constexpr Color32(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
		: r(r), g(g), b(b), a(a)
	{

	}
    constexpr Color32(uint32_t color)
        : r((color >> 24) & 0xFF), g((color >> 16) & 0xFF), b((color >> 8) & 0xFF), a(color & 0xFF)
    {
        
    }

	NO_DISCARD FORCE_INLINE constexpr operator Color() const;
	FORCE_INLINE constexpr Color32& operator=(const Color&);
    
    NO_DISCARD FORCE_INLINE constexpr operator uint32_t() const;
    FORCE_INLINE constexpr Color32& operator=(uint32_t);
    
    NO_DISCARD FORCE_INLINE constexpr bool operator==(const Color32& other) const
    {
        return r == other.r && g == other.g && b == other.b && a == other.a;
    }
    
    NO_DISCARD FORCE_INLINE constexpr bool operator!=(const Color32& other) const
    {
        return !(*this == other);
    }

};

NO_DISCARD FORCE_INLINE constexpr Color::operator Color32() const
{
	return Color32
	{
		static_cast<uint8_t>(Math::Round(r * 255.0f)),
		static_cast<uint8_t>(Math::Round(g * 255.0f)),
		static_cast<uint8_t>(Math::Round(b * 255.0f)),
		static_cast<uint8_t>(Math::Round(a * 255.0f))
	};
}

FORCE_INLINE constexpr Color& Color::operator=(const Color32& color)
{
	return *this = static_cast<Color>(color);
}

NO_DISCARD FORCE_INLINE constexpr Color32::operator Color() const
{
	return Color
	{
		static_cast<float>(r) / 255.0f,
		static_cast<float>(g) / 255.0f,
		static_cast<float>(b) / 255.0f,
		static_cast<float>(a) / 255.0f
	};
}

FORCE_INLINE constexpr Color32& Color32::operator=(const Color& color)
{
	return *this = static_cast<Color32>(color);
}

NO_DISCARD FORCE_INLINE constexpr Color32::operator uint32_t() const
{
	return (static_cast<uint32_t>(r) << 24) | (static_cast<uint32_t>(g) << 16) | (static_cast<uint32_t>(b) << 8) | static_cast<uint32_t>(a);
}

FORCE_INLINE constexpr Color32& Color32::operator=(uint32_t color)
{
    return *this = static_cast<Color32>(color);
}

NO_DISCARD FORCE_INLINE constexpr Color32 Mix(Color32 c0, Color32 c1, float a)
{
	return Color32
	{
		static_cast<uint8_t>(static_cast<float>(c0.r) * (1.0f - a) + static_cast<float>(c1.r) * a),
		static_cast<uint8_t>(static_cast<float>(c0.g) * (1.0f - a) + static_cast<float>(c1.g) * a),
		static_cast<uint8_t>(static_cast<float>(c0.b) * (1.0f - a) + static_cast<float>(c1.b) * a),
		static_cast<uint8_t>(static_cast<float>(c0.a) * (1.0f - a) + static_cast<float>(c1.a) * a)
	};
}

} // namespace Gleam

GLEAM_TYPE(Gleam::Color, Guid("9C2ADB39-C783-4826-9FD4-DE1062A08094"))
GLEAM_END

GLEAM_TYPE(Gleam::Color32, Guid("88FA9547-6F5F-45C4-8AE1-135B27AD29F7"))
    GLEAM_FIELD(r, Serializable())
    GLEAM_FIELD(g, Serializable())
    GLEAM_FIELD(b, Serializable())
    GLEAM_FIELD(a, Serializable())
GLEAM_END
