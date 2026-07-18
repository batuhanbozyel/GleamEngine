#pragma once
#include "Renderer/TextureDescriptor.h"

namespace GEditor {

struct RawTexture;

namespace TextureTools {

Gleam::TArray<Gleam::BufferRange> CalculateSubresourceRanges(const RawTexture& texture, uint32_t mipLevels);
Gleam::BinaryBuffer GenerateMipmaps(const RawTexture& texture);

} // namespace TextureTools

} // namespace GEditor
