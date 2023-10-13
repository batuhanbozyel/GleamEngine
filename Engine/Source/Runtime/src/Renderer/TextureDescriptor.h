//
//  TextureDescriptor.h
//  GleamEngine
//
//  Created by Batuhan Bozyel on 14.03.2023.
//

#pragma once
#include "TextureFormat.h"

namespace Gleam {

enum class TextureType
{
    Texture2D,
    TextureCube,
    RenderTexture
};

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
    TextureType type = TextureType::Texture2D;
    uint32_t sampleCount = 1;
    bool useMipMap = false;
    
    bool operator==(const TextureDescriptor& other) const
    {
        return  size == other.size &&
                format == other.format &&
                usage == other.usage &&
                type == other.type &&
                sampleCount == other.sampleCount &&
                useMipMap == other.useMipMap;
    }
};

struct RenderTextureDescriptor : public TextureDescriptor
{
	Color clearColor = Color::clear;
	uint32_t clearStencil = 0u;
	float clearDepth = 1.0f;
	bool clearBuffer = false;
    
    RenderTextureDescriptor()
        : TextureDescriptor()
    {
        type = TextureType::RenderTexture;
    }
    
    RenderTextureDescriptor(const TextureDescriptor& descriptor)
        : TextureDescriptor(descriptor)
    {
        type = TextureType::RenderTexture;
    }
};

} // namespace Gleam

template <>
struct std::hash<Gleam::TextureDescriptor>
{
    size_t operator()(const Gleam::TextureDescriptor& descriptor) const
    {
        size_t hash = 0;
        Gleam::hash_combine(hash, static_cast<int>(descriptor.size.width));
        Gleam::hash_combine(hash, static_cast<int>(descriptor.size.height));
        Gleam::hash_combine(hash, descriptor.format);
        Gleam::hash_combine(hash, descriptor.usage);
        Gleam::hash_combine(hash, descriptor.type);
        Gleam::hash_combine(hash, descriptor.sampleCount);
        Gleam::hash_combine(hash, descriptor.useMipMap);
        return hash;
    }
};
