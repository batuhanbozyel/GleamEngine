//
//  TextureDescriptor.h
//  GleamEngine
//
//  Created by Batuhan Bozyel on 14.03.2023.
//

#pragma once
#include "TextureFormat.h"

#include "Math/Size.h"
#include "Math/Color.h"

#include "Container/String.h"
#include "Container/Array.h"
#include "Container/Hash.h"

namespace Gleam {

GENUM(TextureDimension, "7A1CDA2E-8B61-4558-9255-B919E70E92F7", Serializable)
{
	GITEM(Texture2D, "DE76EACC-A162-45BD-9F88-9C5F1A3B3EC7"),
	GITEM(TextureCube, "E8541EC5-7B16-4FF5-BF9A-27C8AFF5CB83")
};

GENUM(TextureUsage, "7EFFFEDD-F5B2-443B-9888-49C88D41779B", Serializable)
{
	GITEM(Sampled, "3D7FA6A5-D83A-4CAB-A5E7-43D38A768D70"),
	GITEM(Storage, "6CAA9FB4-89FE-4D2F-9CE4-E3F0A2E9F6A2"),
	GITEM(Attachment, "CA74DE24-9E22-4DB9-84AE-67E72F02D1E6")
};

enum TextureUsageFlag
{
	TextureUsage_Sampled = BIT(static_cast<uint32_t>(TextureUsage::Sampled)),
	TextureUsage_Storage = BIT(static_cast<uint32_t>(TextureUsage::Storage)),
	TextureUsage_Attachment = BIT(static_cast<uint32_t>(TextureUsage::Attachment))
};
typedef uint32_t TextureUsageFlagBits;

GSTRUCT(TextureDescriptor, "5B36D630-8A7E-47BE-A9F0-1702AB9F9C8C", Serializable)
{
	GFIELD("A45F7E8C-D2B9-4D63-9BA3-1E7C24C5BF7A", Serializable)
	TString name;

	GFIELD("C3F6A7D1-9E23-48B7-A452-6D1F2E8C9A5B", Serializable)
	Size size = Size::zero;

	GFIELD("8B9D2F4C-E75A-4C63-8D91-F2B7A9E3D1C6", Serializable)
	TextureFormat format = TextureFormat::R8G8B8A8_UNorm;

	GFIELD("E6D2C8B5-A793-4F61-B8E2-D7A9C5F4E3B1", Serializable)
	TextureUsageFlagBits usage = TextureUsage_Sampled;

	GFIELD("1F8E7D6C-B5A4-4F32-9E1D-C8B7A6F5E4D3", Serializable)
	TextureDimension dimension = TextureDimension::Texture2D;

	GFIELD("3C7B6A5D-E9F8-4A21-B7C6-D5E4F3A2C1B9", Serializable)
	bool useMipMap = false;

	bool operator==(const TextureDescriptor & other) const
	{
		return  size == other.size &&
				format == other.format &&
				usage == other.usage &&
				dimension == other.dimension &&
				useMipMap == other.useMipMap;
	}
};

GSTRUCT(Texture2DDescriptor, "CC19ED9A-2B9F-4258-B0E5-1F0EB34373A1", Serializable)
	: TextureDescriptor
{
	GFIELD("F1E2D3C4-B5A6-4789-B1C2-D3E4F5A6B7C8", Serializable)
	TArray<uint8_t> pixels;
};

GSTRUCT(RenderTextureDescriptor, "7B6A5D4C-3E2F-4180-9D8C-7B6A5D4C3E2F", Serializable)
	: TextureDescriptor
{
	GFIELD("8C7B6A5D-4E3F-4291-AE8D-8C7B6A5D4E3F", Serializable)
	Color clearColor = Color::clear;

	GFIELD("9D8C7B6A-5E4F-43A2-BF9E-9D8C7B6A5E4F", Serializable)
	uint32_t clearStencil = 0u;

	GFIELD("AE9D8C7B-6F5E-44B3-C0AF-AE9D8C7B6F5E", Serializable)
	float clearDepth = 1.0f;

	GFIELD("BFA0E9D8-7C6F-45C4-D1B0-BFA0E9D87C6F", Serializable)
	bool clearBuffer = false;

	RenderTextureDescriptor()
		: TextureDescriptor()
	{
		usage |= TextureUsage_Attachment;
	}

	RenderTextureDescriptor(const TextureDescriptor& descriptor)
		: TextureDescriptor(descriptor)
	{
		usage |= TextureUsage_Attachment;
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
        Gleam::hash_combine(hash, descriptor.dimension);
        Gleam::hash_combine(hash, descriptor.useMipMap);
        return hash;
    }
};

template <>
struct eastl::hash<Gleam::TextureDescriptor>
{
	size_t operator()(const Gleam::TextureDescriptor& descriptor) const
	{
		return std::hash<Gleam::TextureDescriptor>()(descriptor);
	}
};