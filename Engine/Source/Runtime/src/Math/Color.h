#pragma once

namespace Gleam {

struct Color : public Vector4
{
	static const Color Clear;
	static const Color Black;
	static const Color White;
	static const Color Gray;
	static const Color Red;
	static const Color Green;
	static const Color Blue;
	static const Color Cyan;
	static const Color Magenta;
	static const Color Yellow;

	constexpr float MaxComponent() const
	{
		return Math::Max(r, Math::Max(g, Math::Max(b, a)));
	}

	MATH_INLINE static constexpr Color HSVToRGB(float h, float s, float v)
	{
		constexpr Vector4 K = Vector4(1.0f, 2.0f / 3.0f, 1.0f / 3.0f, 3.0f);

		return Math::Mix(K.x, Math::Clamp(Math::Abs(Math::Fract(Vector3(K.x + h, K.y + h, K.z + h)) * 6.0f - K.w) - K.x, 0.0f, 1.0f), s) * v;
	}

	MATH_INLINE static constexpr Color HSVToRGB(const Color& hsv)
	{
		constexpr Vector4 K = Vector4(1.0f, 2.0f / 3.0f, 1.0f / 3.0f, 3.0f);

		return Math::Mix(K.x, Math::Clamp(Math::Abs(Math::Fract(Vector3(K.x + hsv.x, K.y + hsv.x, K.z + hsv.x)) * 6.0f - K.w) - K.x, 0.0f, 1.0f), hsv.y) * hsv.z;
	}

	MATH_INLINE static constexpr Color RGBToHSV(float r, float g, float b)
	{
		constexpr Vector4 K = Vector4(0.0f, -1.0f / 3.0f, 2.0f / 3.0f, -1.0f);

		Vector4 p = Math::Mix(Vector4(b, g, K.w, K.z), Vector4(g, b, K.x, K.y), Math::Step(b, g));
		Vector4 q = Math::Mix(Vector4(p.x, p.y, p.w, r), Vector4(r, p.y, p.z, p.x), Math::Step(p.x, r));
		float d = q.x - Math::Min(q.w, q.y);

		return Color(Math::Abs(q.z + (q.w - q.y) / (6.0f * d + Math::Epsilon)), d / (q.x + Math::Epsilon), q.x);
	}

	MATH_INLINE static constexpr Color RGBToHSV(const Color& color)
	{
		constexpr Vector4 K = Vector4(0.0f, -1.0f / 3.0f, 2.0f / 3.0f, -1.0f);

		Vector4 p = Math::Mix(Vector4(color.b, color.g, K.w, K.z), Vector4(color.g, color.b, K.x, K.y), Math::Step(color.b, color.g));
		Vector4 q = Math::Mix(Vector4(p.x, p.y, p.w, color.r), Vector4(color.r, p.y, p.z, p.x), Math::Step(p.x, color.r));
		float d = q.x - Math::Min(q.w, q.y);

		return Color(Math::Abs(q.z + (q.w - q.y) / (6.0f * d + Math::Epsilon)), d / (q.x + Math::Epsilon), q.x);
	}

	// Constructors
	Color() = default;
	constexpr Color(Color&&) = default;
	constexpr Color(const Color&) = default;

	constexpr Color(float v)
		: Vector4(v)
	{

	}
	constexpr Color(float r, float g, float b)
		: Vector4(r, g, b, 1.0f)
	{

	}
	constexpr Color(float r, float g, float b, float a)
		: Vector4(r, g, b, a)
	{

	}
	constexpr Color(const TArray<float, 3>& color)
		: Vector4(color[0], color[1], color[2], 1.0f)
	{

	}
	constexpr Color(const TArray<float, 4>& color)
		: Vector4(color)
	{

	}
	constexpr Color(const Vector3& color)
		: Vector4(color.r, color.g, color.b, 1.0f)
	{

	}
	constexpr Color(const Vector4& color)
		: Vector4(color)
	{

	}
	constexpr Color(Vector4&& color)
		: Vector4(std::move(color))
	{

	}

	// Operator overloads
	MATH_INLINE constexpr Color& operator=(Color&&) = default;
	MATH_INLINE constexpr Color& operator=(const Color&) = default;

	MATH_INLINE constexpr Color& operator=(const Vector4& color)
	{
		value = std::move(color.value);
		return *this;
	}
	MATH_INLINE constexpr Color& operator=(Vector4&& color)
	{
		value = std::move(color.value);
		return *this;
	}

};

struct Color32
{
	std::byte r, g, b, a;

	Color32() = default;
	constexpr Color32(std::byte r, std::byte g, std::byte b, std::byte a)
		: r(r), g(g), b(b), a(a)
	{

	}

};

}