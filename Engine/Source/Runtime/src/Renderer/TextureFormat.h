#pragma once

namespace Gleam {

enum class TextureFormat
{
	None,

	R8G8B8A8_SRGB,

	R8_UNorm,
	R8G8_UNorm,
	R8G8B8A8_UNorm,

	R8_SNorm,
	R8G8_SNorm,
	R8G8B8A8_SNorm,

	R8_UInt,
	R8G8_UInt,
	R8G8B8A8_UInt,

	R8_SInt,
	R8G8_SInt,
	R8G8B8A8_SInt,

	R16_UNorm,
	R16G16_UNorm,
	R16G16B16A16_UNorm,

	R16_SNorm,
	R16G16_SNorm,
	R16G16B16A16_SNorm,

	R16_UInt,
	R16G16_UInt,
	R16G16B16A16_UInt,

	R16_SInt,
	R16G16_SInt,
	R16G16B16A16_SInt,

	R16_SFloat,
	R16G16_SFloat,
	R16G16B16A16_SFloat,

	R32_UInt,
	R32G32_UInt,
	R32G32B32A32_UInt,

	R32_SInt,
	R32G32_SInt,
	R32G32B32A32_SInt,

	R32_SFloat,
	R32G32_SFloat,
	R32G32B32A32_SFloat,
    
    B8G8R8A8_SRGB,
    B8G8R8A8_UNorm,
    
    // Depth - Stencil formats
    D16_UNorm,
    D32_SFloat,
    D24_UNorm_S8_UInt,
    D32_SFloat_S8_UInt
};

namespace Utils {

static constexpr size_t GetTextureFormatSizeInBytes(TextureFormat format)
{
	switch (format)
	{
		case TextureFormat::R8G8B8A8_SRGB: return 4;

		case TextureFormat::R8_UNorm: return 1;
		case TextureFormat::R8G8_UNorm: return 2;
		case TextureFormat::R8G8B8A8_UNorm: return 4;

		case TextureFormat::R8_SNorm: return 1;
		case TextureFormat::R8G8_SNorm: return 2;
		case TextureFormat::R8G8B8A8_SNorm: return 4;

		case TextureFormat::R8_UInt: return 1;
		case TextureFormat::R8G8_UInt: return 2;
		case TextureFormat::R8G8B8A8_UInt: return 4;

		case TextureFormat::R8_SInt: return 1;
		case TextureFormat::R8G8_SInt: return 2;
		case TextureFormat::R8G8B8A8_SInt: return 4;

		case TextureFormat::R16_UNorm: return 2;
		case TextureFormat::R16G16_UNorm: return 4;
		case TextureFormat::R16G16B16A16_UNorm: return 8;

		case TextureFormat::R16_SNorm: return 2;
		case TextureFormat::R16G16_SNorm: return 4;
		case TextureFormat::R16G16B16A16_SNorm: return 8;

		case TextureFormat::R16_UInt: return 2;
		case TextureFormat::R16G16_UInt: return 4;
		case TextureFormat::R16G16B16A16_UInt: return 8;

		case TextureFormat::R16_SInt: return 2;
		case TextureFormat::R16G16_SInt: return 4;
		case TextureFormat::R16G16B16A16_SInt: return 8;

		case TextureFormat::R16_SFloat: return 2;
		case TextureFormat::R16G16_SFloat: return 4;
		case TextureFormat::R16G16B16A16_SFloat: return 8;

		case TextureFormat::R32_UInt: return 4;
		case TextureFormat::R32G32_UInt: return 8;
		case TextureFormat::R32G32B32A32_UInt: return 16;

		case TextureFormat::R32_SInt: return 4;
		case TextureFormat::R32G32_SInt: return 8;
		case TextureFormat::R32G32B32A32_SInt: return 16;

		case TextureFormat::R32_SFloat: return 4;
		case TextureFormat::R32G32_SFloat: return 8;
		case TextureFormat::R32G32B32A32_SFloat: return 16;

		case TextureFormat::B8G8R8A8_SRGB: return 4;
		case TextureFormat::B8G8R8A8_UNorm: return 4;

        // Depth - Stencil formats
		case TextureFormat::D16_UNorm: return 2;
		case TextureFormat::D32_SFloat: return 4;
        case TextureFormat::D24_UNorm_S8_UInt: return 4;
		case TextureFormat::D32_SFloat_S8_UInt: return 5;

		default: return 0;
	}
}

static constexpr bool IsColorFormat(TextureFormat format)
{
	switch (format)
	{
		case TextureFormat::R8G8B8A8_SRGB:

		case TextureFormat::R8_UNorm:
		case TextureFormat::R8G8_UNorm:
		case TextureFormat::R8G8B8A8_UNorm:

		case TextureFormat::R8_SNorm:
		case TextureFormat::R8G8_SNorm:
		case TextureFormat::R8G8B8A8_SNorm:

		case TextureFormat::R8_UInt:
		case TextureFormat::R8G8_UInt:
		case TextureFormat::R8G8B8A8_UInt:

		case TextureFormat::R8_SInt:
		case TextureFormat::R8G8_SInt:
		case TextureFormat::R8G8B8A8_SInt:

		case TextureFormat::R16_UNorm:
		case TextureFormat::R16G16B16A16_UNorm:

		case TextureFormat::R16_SNorm:
		case TextureFormat::R16G16_SNorm:
		case TextureFormat::R16G16B16A16_SNorm:

		case TextureFormat::R16_UInt:
		case TextureFormat::R16G16_UInt:
		case TextureFormat::R16G16B16A16_UInt:

		case TextureFormat::R16_SInt:
		case TextureFormat::R16G16_SInt:
		case TextureFormat::R16G16B16A16_SInt:

		case TextureFormat::R16_SFloat:
		case TextureFormat::R16G16_SFloat:
		case TextureFormat::R16G16B16A16_SFloat:

		case TextureFormat::R32_UInt:
		case TextureFormat::R32G32_UInt:
		case TextureFormat::R32G32B32A32_UInt:

		case TextureFormat::R32_SInt:
		case TextureFormat::R32G32_SInt:
		case TextureFormat::R32G32B32A32_SInt:

		case TextureFormat::R32_SFloat:
		case TextureFormat::R32G32_SFloat:
		case TextureFormat::R32G32B32A32_SFloat:

		case TextureFormat::B8G8R8A8_SRGB:
		case TextureFormat::B8G8R8A8_UNorm: return true;
		default: return false;
	}
}

static constexpr bool IsDepthStencilFormat(TextureFormat format)
{
	switch (format)
	{
		case TextureFormat::D32_SFloat:
        case TextureFormat::D24_UNorm_S8_UInt:
		case TextureFormat::D32_SFloat_S8_UInt: return true;
		default: return false;
	}
}

static constexpr bool IsDepthFormat(TextureFormat format)
{
    switch (format)
    {
        case TextureFormat::D16_UNorm:
        case TextureFormat::D32_SFloat:
        case TextureFormat::D24_UNorm_S8_UInt:
        case TextureFormat::D32_SFloat_S8_UInt: return true;
        default: return false;
    }
}

} // namespace Utils
    
} // namespace Gleam

GLEAM_ENUM(Gleam::TextureFormat, Guid("B87B25FD-841E-45C8-A2D4-27540B268A52"))
