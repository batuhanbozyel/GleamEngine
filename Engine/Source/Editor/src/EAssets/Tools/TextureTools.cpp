#include "TextureTools.h"
#include "Renderer/Texture.h"
#include "EAssets/TextureSource.h"

#define STB_IMAGE_RESIZE2_IMPLEMENTATION
#include <stb_image_resize2.h>

using namespace GEditor;

Gleam::TArray<Gleam::TextureSubresource> TextureTools::GenerateMipmaps(const RawTexture& texture)
{
	Gleam::TArray<Gleam::TextureSubresource> subresources(Gleam::Texture::CalculateMipLevels(Gleam::Size((float)texture.width, (float)texture.height)));

	auto& firstSubresource = subresources[0];
	firstSubresource.pixels.resize(texture.width * texture.height * Gleam::Utils::GetTextureFormatSizeInBytes(texture.format));
	memcpy(firstSubresource.pixels.data(), texture.pixels, firstSubresource.pixels.size());

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
			return subresources;
		}
	}

	for (uint32_t mip = 1; mip < subresources.size(); ++mip)
	{
		uint32_t dstMipWidth = Gleam::Math::Max(texture.width >> mip, 1);
		uint32_t dstMipHeight = Gleam::Math::Max(texture.height >> mip, 1);

		uint32_t srcMipWidth = Gleam::Math::Max(texture.width >> (mip - 1), 1);
		uint32_t srcMipHeight = Gleam::Math::Max(texture.height >> (mip - 1), 1);

		auto& subresource = subresources[mip];
		subresource.pixels.resize(dstMipWidth * dstMipHeight * Gleam::Utils::GetTextureFormatSizeInBytes(texture.format));

		stbir_resize(subresources[mip - 1].pixels.data(),
					 srcMipWidth,
					 srcMipHeight,
					 0,
					 subresource.pixels.data(),
					 dstMipWidth,
					 dstMipHeight,
					 0,
					 pixelLayout,
					 dataType,
					 STBIR_EDGE_CLAMP,
					 STBIR_FILTER_BOX);
	}
	return subresources;
}