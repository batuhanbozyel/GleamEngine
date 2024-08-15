#include "TextureBaker.h"

using namespace GEditor;

TextureBaker::TextureBaker(const Gleam::TextureDescriptor& descriptor)
	: mDescriptor(descriptor)
{
	
}

Gleam::AssetReference TextureBaker::Bake(const Gleam::Filesystem::Path& directory) const
{
    const auto& typeGuid = Gleam::Reflection::GetClass<Gleam::TextureDescriptor>().Guid();
    auto guid = Gleam::Guid::NewGuid();
    Gleam::AssetReference asset{ .type = typeGuid, .guid = guid };
    // TODO:
    return asset;
}

Gleam::TString TextureBaker::Filename() const
{
	return mDescriptor.name + Gleam::Asset::extension().data();
}
