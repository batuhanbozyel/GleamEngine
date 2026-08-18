#include "TextureBaker.h"
#include "EAssets/AssetRegistry.h"
#include "EAssets/AssetWriter.h"

#include "Assets/Asset.h"

using namespace GEditor;

TextureBaker::TextureBaker(const Gleam::Texture2DDescriptor& descriptor)
	: mDescriptor(descriptor)
{

}

void TextureBaker::Bake(const Gleam::Path& directory, const AssetItem& item) const
{
	BinaryAssetWriter writer;
	writer.Write(directory, item, mDescriptor);
}

Gleam::TString TextureBaker::Name() const
{
	return mDescriptor.name;
}

Gleam::Guid TextureBaker::TypeGuid() const
{
    return Gleam::Reflection::GetClass<decltype(mDescriptor)>().Guid();
}
