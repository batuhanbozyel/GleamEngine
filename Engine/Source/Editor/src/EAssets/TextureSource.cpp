#include "Gleam.h"
#include "TextureSource.h"
#include "Bakers/TextureBaker.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

using namespace GEditor;

bool TextureSource::Import(const Gleam::Filesystem::Path& path, const ImportSettings& settings)
{
	RawTexture texture;
	stbi_info(path.string().c_str(), &texture.width, &texture.height, &texture.channels);
	
	if (texture.channels == 3)
	{
		texture.channels = 4;
	}
	
	Gleam::TextureFormat format;
	if (settings.linear)
	{
		texture.pixels = stbi_load(path.string().c_str(), &texture.width, &texture.height, nullptr, texture.channels);
		switch (texture.channels)
		{
			case 1:
				format = Gleam::TextureFormat::R8_UNorm;
			case 2:
				format = Gleam::TextureFormat::R8G8_UNorm;
			case 4:
				format = Gleam::TextureFormat::R8G8B8A8_UNorm;
			default:
				format = Gleam::TextureFormat::None;
		}
	}
	else
	{
		// TODO: convert to half precision
		texture.pixels = stbi_loadf(path.string().c_str(), &texture.width, &texture.height, nullptr, texture.channels);
		switch (texture.channels)
		{
			case 1:
				format = Gleam::TextureFormat::R32_SFloat;
			case 2:
				format = Gleam::TextureFormat::R32G32_SFloat;
			case 4:
				format = Gleam::TextureFormat::R32G32B32A32_SFloat;
			default:
				format = Gleam::TextureFormat::None;
		}
	}
	
	if (texture.pixels == nullptr)
	{
		GLEAM_ASSERT(false, "Failed to load texture file");
		return false;
	}
	
	if (format == Gleam::TextureFormat::None)
	{
		GLEAM_ASSERT(false, "Unsupported number of texture components");
		stbi_image_free(texture.pixels);
		return false;
	}
	
	Gleam::Texture2DDescriptor descriptor;
	descriptor.format = format;
	descriptor.name = path.stem().string();
	descriptor.size.width = static_cast<float>(texture.width);
	descriptor.size.height = static_cast<float>(texture.height);
	descriptor.dimension = Gleam::TextureDimension::Texture2D;
	descriptor.pixels.resize(texture.width * texture.height * Gleam::Utils::GetTextureFormatSizeInBytes(format));
	memcpy(descriptor.pixels.data(), texture.pixels, descriptor.pixels.size());
	EmplaceBaker<TextureBaker>(descriptor);
	
	stbi_image_free(texture.pixels);
	return true;
}
