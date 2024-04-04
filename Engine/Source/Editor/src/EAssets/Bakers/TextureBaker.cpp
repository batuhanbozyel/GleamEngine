#include "TextureBaker.h"

using namespace GEditor;

TextureBaker::TextureBaker(const Gleam::TextureDescriptor& descriptor)
	: mDescriptor(descriptor)
{
	
}

Gleam::Asset TextureBaker::Bake(const Gleam::Filesystem::path& directory) const
{
    const auto& typeGuid = Gleam::Reflection::GetClass<Gleam::TextureDescriptor>().Guid();
    auto guid = Gleam::Guid::NewGuid();
    Gleam::Asset asset(guid, typeGuid);
    // TODO:
    return asset;
}

Gleam::TString TextureBaker::Filename() const
{
	return mDescriptor.name + ".asset";
}
