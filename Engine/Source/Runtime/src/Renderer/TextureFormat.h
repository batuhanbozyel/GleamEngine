#pragma once
#include <Reflection/Macro.h>

#include <cstddef>

namespace Gleam {

GENUM(TextureFormat, "B87B25FD-841E-45C8-A2D4-27540B268A52", Serializable)
{
	GITEM(None, "C4A06204-B57D-4A31-8189-BB4D644E5BF6"),

	GITEM(R8G8B8A8_SRGB, "F2A8D156-E371-42E4-B240-1306A5AB9E7A"),

	GITEM(R8_UNorm, "5B4D1F05-E8A3-43D6-8D12-81F7E4D9C9B6"),
	GITEM(R8G8_UNorm, "C8E5A703-4D90-47DB-A326-F83C96CB0E5D"),
	GITEM(R8G8B8A8_UNorm, "A1B2C3D4-E5F6-4A5B-8C9D-1E2F3A4B5C6D"),

	GITEM(R8_SNorm, "6C7D8E9F-A0B1-42C3-9D4E-5F6A7B8C9D0E"),
	GITEM(R8G8_SNorm, "1A2B3C4D-5E6F-47A8-9B0C-1D2E3F4A5B6C"),
	GITEM(R8G8B8A8_SNorm, "7D8E9F0A-1B2C-43D4-9E5F-6A7B8C9D0E1F"),

	GITEM(R8_UInt, "2A3B4C5D-6E7F-48A9-9B0C-1D2E3F4A5B6C"),
	GITEM(R8G8_UInt, "8D9E0F1A-2B3C-44D5-9E6F-7A8B9C0D1E2F"),
	GITEM(R8G8B8A8_UInt, "3A4B5C6D-7E8F-40A1-9B2C-3D4E5F6A7B8C"),

	GITEM(R8_SInt, "9D0E1F2A-3B4C-45D6-9E7F-8A9B0C1D2E3F"),
	GITEM(R8G8_SInt, "4A5B6C7D-8E9F-41A2-9B3C-4D5E6F7A8B9C"),
	GITEM(R8G8B8A8_SInt, "0D1E2F3A-4B5C-46D7-9E8F-9A0B1C2D3E4F"),

	GITEM(R16_UNorm, "5A6B7C8D-9E0F-47A1-9B2C-5D6E7F8A9B0C"),
	GITEM(R16G16_UNorm, "1D2E3F4A-5B6C-48D7-9E8F-0A1B2C3D4E5F"),
	GITEM(R16G16B16A16_UNorm, "6A7B8C9D-0E1F-49A2-9B3C-4D5E6F7A8B9C"),

	GITEM(R16_SNorm, "2D3E4F5A-6B7C-40D8-9E9F-0A1B2C3D4E5F"),
	GITEM(R16G16_SNorm, "7A8B9C0D-1E2F-41A3-9B4C-5D6E7F8A9B0C"),
	GITEM(R16G16B16A16_SNorm, "3D4E5F6A-7B8C-42D9-9E0F-1A2B3C4D5E6F"),

	GITEM(R16_UInt, "8A9B0C1D-2E3F-43A4-9B5C-6D7E8F9A0B1C"),
	GITEM(R16G16_UInt, "4D5E6F7A-8B9C-44D0-9E1F-2A3B4C5D6E7F"),
	GITEM(R16G16B16A16_UInt, "9A0B1C2D-3E4F-45A5-9B6C-7D8E9F0A1B2C"),

	GITEM(R16_SInt, "5D6E7F8A-9B0C-46D1-9E2F-3A4B5C6D7E8F"),
	GITEM(R16G16_SInt, "0A1B2C3D-4E5F-47A6-9B7C-8D9E0F1A2B3C"),
	GITEM(R16G16B16A16_SInt, "6D7E8F9A-0B1C-48D2-9E3F-4A5B6C7D8E9F"),

	GITEM(R16_SFloat, "1A2B3C4D-5E6F-49A7-9B8C-9D0E1F2A3B4C"),
	GITEM(R16G16_SFloat, "7D8E9F0A-1B2C-40D3-9E4F-5A6B7C8D9E0F"),
	GITEM(R16G16B16A16_SFloat, "2A3B4C5D-6E7F-41A8-9B9C-0D1E2F3A4B5C"),

	GITEM(R32_UInt, "8D9E0F1A-2B3C-42D4-9E5F-6A7B8C9D0E1F"),
	GITEM(R32G32_UInt, "3A4B5C6D-7E8F-43D9-9E0F-1A2B3C4D5E6F"),
	GITEM(R32G32B32A32_UInt, "9D0E1F2A-3B4C-44D5-9E6F-7A8B9C0D1E2F"),

	GITEM(R32_SInt, "4A5B6C7D-8E9F-45D0-9E1F-2A3B4C5D6E7F"),
	GITEM(R32G32_SInt, "0D1E2F3A-4B5C-46D6-9E7F-8A9B0C1D2E3F"),
	GITEM(R32G32B32A32_SInt, "5A6B7C8D-9E0F-47D1-9E2F-3A4B5C6D7E8F"),

	GITEM(R32_SFloat, "1D2E3F4A-5B6C-48D7-9E8F-9A0B1C2D3E4F"),
	GITEM(R32G32_SFloat, "6A7B8C9D-0E1F-49D2-9E3F-4A5B6C7D8E9F"),
	GITEM(R32G32B32A32_SFloat, "2D3E4F5A-6B7C-40D8-9E9F-0A1B2C3D4E5F"),

	GITEM(B8G8R8A8_SRGB, "7A8B9C0D-1E2F-41D3-9E4F-5A6B7C8D9E0F"),
	GITEM(B8G8R8A8_UNorm, "3D4E5F6A-7B8C-42D9-9E0F-1A2B3C4D5E6F"),

	GITEM(R9G9B9E5_SFloat, "ABF5A9CE-2F3B-4E70-B806-8B84DFEB3FAC"),
	GITEM(R11G11B10_SFloat, "4D50C960-E388-4F2A-8592-88249F550138"),

	// Depth - Stencil formats
	GITEM(D16_UNorm, "8A9B0C1D-2E3F-43D4-9E5F-6A7B8C9D0E1F"),
	GITEM(D32_SFloat, "4D5E6F7A-8B9C-44D0-9E1F-2A3B4C5D6E7F"),
	GITEM(D24_UNorm_S8_UInt, "9A0B1C2D-3E4F-45D5-9E6F-7A8B9C0D1E2F"),
	GITEM(D32_SFloat_S8_UInt, "5D6E7F8A-9B0C-46D1-9E2F-3A4B5C6D7E8F")
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

		case TextureFormat::R9G9B9E5_SFloat: return 4;
		case TextureFormat::R11G11B10_SFloat: return 4;

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
		case TextureFormat::B8G8R8A8_UNorm:

		case TextureFormat::R9G9B9E5_SFloat:
		case TextureFormat::R11G11B10_SFloat: return true;
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
