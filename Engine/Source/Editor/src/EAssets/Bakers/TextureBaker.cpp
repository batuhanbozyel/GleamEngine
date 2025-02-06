#include "TextureBaker.h"
#include "EAssets/AssetRegistry.h"

using namespace GEditor;

TextureBaker::TextureBaker(const Gleam::Texture2DDescriptor& descriptor)
	: mDescriptor(descriptor)
{
	
}

void TextureBaker::Bake(const Gleam::Filesystem::Path& directory, const AssetItem& item) const
{
	auto filename = item.reference.guid.ToString() + Gleam::Asset::Extension().data();
	auto file = Gleam::Filesystem::Create(directory / filename, Gleam::FileType::Binary);
	auto accessor = Gleam::Filesystem::WriteAccessor(directory / filename);

	auto serializer = Gleam::BinarySerializer();
	serializer.Serialize(mDescriptor, file.GetStream());
}

Gleam::TString TextureBaker::Filename() const
{
	return mDescriptor.name;
}

Gleam::Guid TextureBaker::TypeGuid() const
{
    return Gleam::Reflection::GetClass<decltype(mDescriptor)>().Guid();
}
