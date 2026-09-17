#pragma once
#include "Renderer/TextureDescriptor.h"

namespace GEditor {

struct RawTexture;

struct TextureData
{
	Gleam::TString name;
	Gleam::Size size = Gleam::Size::zero;
	Gleam::TextureFormat format = Gleam::TextureFormat::None;
	Gleam::BinaryBuffer pixels;
	Gleam::TArray<Gleam::BufferRange> subresources;
};

namespace TextureTools {

Gleam::TArray<Gleam::BufferRange> CalculateSubresourceRanges(const RawTexture& texture, uint32_t mipLevels);
TextureData GenerateMipmaps(const RawTexture& texture);

} // namespace TextureTools

} // namespace GEditor
