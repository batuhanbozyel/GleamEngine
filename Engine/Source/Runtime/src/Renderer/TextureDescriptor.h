//
//  RenderTexture.h
//  GleamEngine
//
//  Created by Batuhan Bozyel on 14.03.2023.
//

#pragma once
#include "TextureFormat.h"

namespace Gleam {

enum class TextureUsage
{
    Sampled,
    Storage,
    Attachment
};

enum TextureUsageFlag
{
    TextureUsage_Sampled = BIT(static_cast<uint32_t>(TextureUsage::Sampled)),
    TextureUsage_Storage = BIT(static_cast<uint32_t>(TextureUsage::Storage)),
    TextureUsage_Attachment = BIT(static_cast<uint32_t>(TextureUsage::Attachment))
};
typedef uint32_t TextureUsageFlagBits;

struct TextureDescriptor
{
    Size size = Size::zero;
    TextureFormat format = TextureFormat::R8G8B8A8_SRGB;
    TextureUsageFlagBits usage = TextureUsage_Sampled;
    uint32_t sampleCount = 1;
    bool useMipMap = false;
    
    bool operator==(const TextureDescriptor& other) const
    {
        return  size == other.size &&
                format == other.format &&
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

