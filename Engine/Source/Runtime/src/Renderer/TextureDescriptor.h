//
//  RenderTexture.h
//  GleamEngine
//
//  Created by Batuhan Bozyel on 14.03.2023.
//

#pragma once
#include "TextureFormat.h"

namespace Gleam {

struct TextureDescriptor
{
    Size size = Size::zero;
    TextureFormat format = TextureFormat::R8G8B8A8_SRGB;
    FilterMode filterMode = FilterMode::Point;
    WrapMode wrapMode = WrapMode::Clamp;
    bool useMipMap = false;
};

struct RenderTextureDescriptor : public TextureDescriptor
{
    uint32_t sampleCount = 1;
};

} // namespace Gleam

