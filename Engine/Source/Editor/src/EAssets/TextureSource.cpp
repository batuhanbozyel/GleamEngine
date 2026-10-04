#include "TextureSource.h"
#include "Bakers/TextureBaker.h"

#include "Tools/TextureTools.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

using namespace GEditor;

bool TextureSource::Import(const Gleam::Path& path, const ImportSettings& settings)
{
	const auto filename = path.String();

	RawTexture texture;
	stbi_info(filename.c_str(), &texture.width, &texture.height, &texture.channels);
	
	if (texture.channels == 3)
	{
		texture.channels = 4;
	}

	// There is no 16-bit sRGB format, so 16-bit sRGB color textures fall back to the 8-bit path
	const bool srgb = texture.channels == 4 and settings.colorSpace == TextureColorSpace::sRGB;
	
	if (stbi_is_hdr(filename.c_str()))
	{
		// TODO: convert to half precision
		texture.pixels = stbi_loadf(filename.c_str(), &texture.width, &texture.height, nullptr, texture.channels);
		switch (texture.channels)
		{
			case 1:
				texture.format = Gleam::TextureFormat::R32_SFloat;
				break;
			case 2:
				texture.format = Gleam::TextureFormat::R32G32_SFloat;
				break;
			case 4:
				texture.format = Gleam::TextureFormat::R32G32B32A32_SFloat;
				break;
			default:
				texture.format = Gleam::TextureFormat::None;
				break;
		}		
	}
	else if (stbi_is_16_bit(filename.c_str()) and srgb == false)
	{
		texture.pixels = stbi_load_16(filename.c_str(), &texture.width, &texture.height, nullptr, texture.channels);
		switch (texture.channels)
		{
			case 1:
				texture.format = Gleam::TextureFormat::R16_UNorm;
				break;
			case 2:
				texture.format = Gleam::TextureFormat::R16G16_UNorm;
				break;
			case 4:
				texture.format = Gleam::TextureFormat::R16G16B16A16_UNorm;
				break;
			default:
				texture.format = Gleam::TextureFormat::None;
				break;
		}
	}
	else
	{
		texture.pixels = stbi_load(filename.c_str(), &texture.width, &texture.height, nullptr, texture.channels);
		switch (texture.channels)
		{
			case 1:
				texture.format = Gleam::TextureFormat::R8_UNorm;
				break;
			case 2:
				texture.format = Gleam::TextureFormat::R8G8_UNorm;
				break;
			case 4:
				texture.format = srgb ? Gleam::TextureFormat::R8G8B8A8_SRGB : Gleam::TextureFormat::R8G8B8A8_UNorm;
				break;
			default:
				texture.format = Gleam::TextureFormat::None;
				break;
		}
	}
	
	if (texture.pixels == nullptr)
	{
		GLEAM_ASSERT(false, "Failed to load texture file");
		return false;
	}
	
	if (texture.format == Gleam::TextureFormat::None)
	{
		GLEAM_ASSERT(false, "Unsupported number of texture components");
		stbi_image_free(texture.pixels);
		return false;
	}
	
	TextureData textureData;
	if (settings.generateMips)
	{
		textureData = TextureTools::GenerateMipmaps(texture);
	}
	else
	{
		textureData.subresources = TextureTools::CalculateSubresourceRanges(texture, 1);
		textureData.pixels = Gleam::BinaryBuffer(texture.pixels, textureData.subresources[0].size);
	}

	textureData.name = path.Stem();
	textureData.format = texture.format;
	textureData.size.width = static_cast<float>(texture.width);
	textureData.size.height = static_cast<float>(texture.height);

	EmplaceBaker<TextureBaker>(std::move(textureData));
	
	stbi_image_free(texture.pixels);
	return true;
}
