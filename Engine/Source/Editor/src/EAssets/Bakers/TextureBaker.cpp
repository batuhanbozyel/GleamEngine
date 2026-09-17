#include "TextureBaker.h"
#include "EAssets/AssetRegistry.h"
#include "EAssets/AssetWriter.h"

#include "Assets/Asset.h"

using namespace GEditor;

TextureBaker::TextureBaker(TextureData&& textureData)
	: mTextureData(std::move(textureData))
{

}

void TextureBaker::Bake(const Gleam::Path& directory, const AssetItem& item) const
{
	Gleam::Texture2DDescriptor descriptor;
	descriptor.name = mTextureData.name;
	descriptor.size = mTextureData.size;
	descriptor.format = mTextureData.format;
	descriptor.dimension = Gleam::TextureDimension::Texture2D;
	descriptor.useMipMap = mTextureData.subresources.size() > 1;
	descriptor.subresources.resize(mTextureData.subresources.size());

	BinaryAssetWriter writer;
	for (uint32_t i = 0; i < mTextureData.subresources.size(); ++i)
	{
		const auto& subresource = mTextureData.subresources[i];
		descriptor.subresources[i].blobSlot = writer.AddBlob<Gleam::TextureSubresourceDescriptor>(Gleam::OffsetPointer(mTextureData.pixels.data, subresource.offset),
																								  subresource.size,
																								  Gleam::AssetPlatform::Common,
																								  Gleam::AssetBackend::Common);
	}
	writer.Write(directory, item, descriptor);
}

Gleam::TString TextureBaker::Name() const
{
	return mTextureData.name;
}

Gleam::Guid TextureBaker::TypeGuid() const
{
    return Gleam::Reflection::GetClass<Gleam::Texture2DDescriptor>().Guid();
}
