#include "TextureSource.h"
#include "Bakers/TextureBaker.h"

#include "Tools/TextureTools.h"
#include "Renderer/Texture.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

using namespace GEditor;

bool TextureSource::Import(const Gleam::Path& path, const ImportSettings& settings)
{
	RawTexture texture;
	stbi_info(path.String().c_str(), &texture.width, &texture.height, &texture.channels);
	
	if (texture.channels == 3)
	{
		texture.channels = 4;
	}
	
	if (settings.hdr)
	{
		// TODO: convert to half precision
		texture.pixels = stbi_loadf(path.String().c_str(), &texture.width, &texture.height, nullptr, texture.channels);
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
	else
	{
		texture.pixels = stbi_load(path.String().c_str(), &texture.width, &texture.height, nullptr, texture.channels);
		switch (texture.channels)
		{
			case 1:
				texture.format = Gleam::TextureFormat::R8_UNorm;
				break;
			case 2:
				texture.format = Gleam::TextureFormat::R8G8_UNorm;
				break;
			case 4:
				texture.format = settings.colorSpace == TextureColorSpace::sRGB ? Gleam::TextureFormat::R8G8B8A8_SRGB : Gleam::TextureFormat::R8G8B8A8_UNorm;
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
	
	Gleam::Texture2DDescriptor descriptor;
	descriptor.format = texture.format;
	descriptor.name = path.Stem();
	descriptor.size.width = static_cast<float>(texture.width);
	descriptor.size.height = static_cast<float>(texture.height);
	descriptor.dimension = Gleam::TextureDimension::Texture2D;
	descriptor.useMipMap = settings.generateMips;

	if (settings.generateMips)
	{
		uint32_t mipLevels = Gleam::Texture::CalculateMipLevels(descriptor.size);
		descriptor.subresources = TextureTools::CalculateSubresourceRanges(texture, mipLevels);
		descriptor.pixels = TextureTools::GenerateMipmaps(texture);
	}
	else
	{
		descriptor.subresources = TextureTools::CalculateSubresourceRanges(texture, 1);
		descriptor.pixels = Gleam::BinaryBuffer(texture.pixels, descriptor.subresources[0].size);
	}
	EmplaceBaker<TextureBaker>(descriptor);
	
	stbi_image_free(texture.pixels);
	return true;
}
