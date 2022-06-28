#pragma once

namespace Gleam {

enum class FilterMode
{
	Point,
	Bilinear,
	Trilinear
};

enum class WrapMode
{
	Repeat,
	Clamp,
	Mirror,
	MirrorOnce
};

enum class TextureFormat
{
	None,

	R8_SRGB,
	R8G8_SRGB,
	R8G8B8_SRGB,
	R8G8B8A8_SRGB,

	R8_UNorm,
	R8G8_UNorm,
	R8G8B8_UNorm,
	R8G8B8A8_UNorm,

	R8_SNorm,
	R8G8_SNorm,
	R8G8B8_SNorm,
	R8G8B8A8_SNorm,

	R8_UInt,
	R8G8_UInt,
	R8G8B8_UInt,
	R8G8B8A8_UInt,

	R8_SInt,
	R8G8_SInt,
	R8G8B8_SInt,
	R8G8B8A8_SInt,

	R16_UNorm,
	R16G16_UNorm,
	R16G16B16_UNorm,
	R16G16B16A16_UNorm,

	R16_SNorm,
	R16G16_SNorm,
	R16G16B16_SNorm,
	R16G16B16A16_SNorm,

	R16_UInt,
	R16G16_UInt,
	R16G16B16_UInt,
	R16G16B16A16_UInt,

	R16_SInt,
	R16G16_SInt,
	R16G16B16_SInt,
	R16G16B16A16_SInt,

	R32_UInt,
	R32G32_UInt,
	R32G32B32_UInt,
	R32G32B32A32_UInt,

	R32_SInt,
	R32G32_SInt,
	R32G32B32_SInt,
	R32G32B32A32_SInt,

	R16_SFloat,
	R16G16_SFloat,
	R16G16B16_SFloat,
	R16G16B16A16_SFloat,

	R32_SFloat,
	R32G32_SFloat,
	R32G32B32_SFloat,
	R32G32B32A32_SFloat,

	B8G8R8_SRGB,
	B8G8R8A8_SRGB,

	B8G8R8_UNorm,
	B8G8R8A8_UNorm,

	B8G8R8_SNorm,
	B8G8R8A8_SNorm,

	B8G8R8_UInt,
	B8G8R8A8_UInt,

	B8G8R8_SInt,
	B8G8R8A8_SInt,

	R4G4B4A4_UNormPack16,
	B4G4R4A4_UNormPack16,

	R5G6B5_UNormPack16,
	B5G6R5_UNormPack16,

	R5G5B5A1_UNormPack16,
	B5G5R5A1_UNormPack16,

	A1R5G5B5_UNormPack16,
	E5B9G9R9_UFloatPack32,

	B10G11R11_UFloatPack32,

	A2B10G10R10_UNormPack32,
	A2B10G10R10_UIntPack32,
	A2B10G10R10_SIntPack32,

	A2R10G10B10_UNormPack32,
	A2R10G10B10_UIntPack32,
	A2R10G10B10_SIntPack32
};

enum class RenderTextureFormat
{
	ARGB32,
	Depth,
	ARGBHalf,
	Shadowmap,
	RGB565,
	ARGB4444,
	ARGB1555,
	Default,
	ARGB2101010,
	DefaultHDR,
	ARGB64,
	ARGBFloat,
	RGFloat,
	RGHalf,
	RFloat,
	RHalf,
	R8,
	ARGBInt,
	RGInt,
	BGRA32,
	RGB111110Float,
	RG32,
	RGBAUShort,
	RG16,
	R16
};

} // namespace Gleam