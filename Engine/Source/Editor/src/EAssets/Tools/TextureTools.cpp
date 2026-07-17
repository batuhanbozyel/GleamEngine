#include "TextureTools.h"
#include "Renderer/Texture.h"
#include "EAssets/TextureSource.h"

#define STB_IMAGE_RESIZE2_IMPLEMENTATION
#include <stb_image_resize2.h>

using namespace GEditor;

Gleam::TArray<Gleam::BufferRange> TextureTools::CalculateSubresourceRanges(const RawTexture& texture, uint32_t mipLevels)
{
	Gleam::TArray<Gleam::BufferRange> subresources(mipLevels);

	uint64_t offset = 0;
	for (uint32_t mip = 0; mip < mipLevels; ++mip)
	{
		uint32_t mipWidth = Gleam::Math::Max(texture.width >> mip, 1);
		uint32_t mipHeight = Gleam::Math::Max(texture.height >> mip, 1);

		auto& subresource = subresources[mip];
		subresource.offset = offset;
		subresource.size = (uint64_t)mipWidth * mipHeight * Gleam::Utils::GetTextureFormatSizeInBytes(texture.format);
		offset += subresource.size;
	}
	return subresources;
}

Gleam::BinaryBuffer TextureTools::GenerateMipmaps(const RawTexture& texture)
{
	auto mipLevels = Gleam::Texture::CalculateMipLevels(Gleam::Size((float)texture.width, (float)texture.height));
	auto subresources = CalculateSubresourceRanges(texture, mipLevels);

	const auto& lastSubresource = subresources.back();
	Gleam::BinaryBuffer pixels(lastSubresource.offset + lastSubresource.size);

	const auto& firstSubresource = subresources[0];
	memcpy(pixels.data, texture.pixels, (size_t)firstSubresource.size);

	stbir_pixel_layout pixelLayout;
	stbir_datatype dataType;

	switch (texture.format)
	{
		case Gleam::TextureFormat::R8_UNorm:
		{
			pixelLayout = STBIR_1CHANNEL;
			dataType = STBIR_TYPE_UINT8;
			break;
		}
		case Gleam::TextureFormat::R16_UNorm:
		{
			pixelLayout = STBIR_1CHANNEL;
			dataType = STBIR_TYPE_UINT16;
			break;
		}
		case Gleam::TextureFormat::R8G8_UNorm:
		{
			pixelLayout = STBIR_2CHANNEL;
			dataType = STBIR_TYPE_UINT8;
			break;
		}
		case Gleam::TextureFormat::R8G8B8A8_UNorm:
		{
			pixelLayout = STBIR_4CHANNEL;
			dataType = STBIR_TYPE_UINT8;
			break;
		}
		case Gleam::TextureFormat::R8G8B8A8_SRGB:
		{
			pixelLayout = STBIR_4CHANNEL;
			dataType = STBIR_TYPE_UINT8_SRGB;
			break;
		}
		case Gleam::TextureFormat::R32_SFloat:
		{
			pixelLayout = STBIR_1CHANNEL;
			dataType = STBIR_TYPE_FLOAT;
			break;
		}
		case Gleam::TextureFormat::R32G32_SFloat:
		{
			pixelLayout = STBIR_2CHANNEL;
			dataType = STBIR_TYPE_FLOAT;
			break;
		}
		case Gleam::TextureFormat::R32G32B32A32_SFloat:
		{
			pixelLayout = STBIR_4CHANNEL;
			dataType = STBIR_TYPE_FLOAT;
			break;
		}
		default:
		{
			GLEAM_ASSERT(false, "Unsupported texture format for mip generation");
			return pixels;
		}
	}

	for (uint32_t mip = 1; mip < subresources.size(); ++mip)
	{
		uint32_t dstMipWidth = Gleam::Math::Max(texture.width >> mip, 1);
		uint32_t dstMipHeight = Gleam::Math::Max(texture.height >> mip, 1);

		uint32_t srcMipWidth = Gleam::Math::Max(texture.width >> (mip - 1), 1);
		uint32_t srcMipHeight = Gleam::Math::Max(texture.height >> (mip - 1), 1);

		stbir_resize(Gleam::OffsetPointer(pixels.data, subresources[mip - 1].offset),
					 srcMipWidth,
					 srcMipHeight,
					 0,
					 Gleam::OffsetPointer(pixels.data, subresources[mip].offset),
					 dstMipWidth,
					 dstMipHeight,
					 0,
					 pixelLayout,
					 dataType,
					 STBIR_EDGE_CLAMP,
					 STBIR_FILTER_BOX);
	}
	return pixels;
}