#pragma once
#include "Renderer/TextureDescriptor.h"

namespace GEditor {

struct RawTexture;

namespace TextureTools {

Gleam::TArray<Gleam::TextureSubresource> GenerateMipmaps(const RawTexture& texture);

} // namespace TextureTools

} // namespace GEditor
