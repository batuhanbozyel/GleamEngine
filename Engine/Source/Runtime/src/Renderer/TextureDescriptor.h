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
    uint32_t sampleCount = 1;
    bool useMipMap = false;
    
    bool operator==(const TextureDescriptor& other) const
    {
        return  size == other.size &&
                format == other.format &&
                filterMode == other.filterMode &&
                wrapMode == other.wrapMode &&
                sampleCount == other.sampleCount &&
                useMipMap == other.useMipMap;
    }
};

struct RenderTextureDescriptor
{
    TextureDescriptor texture;
    Color clearColor = Color::clear;
    uint32_t clearStencil = 0u;
    float clearDepth = 1.0f;
    bool clearBuffer = false;
};

} // namespace Gleam

