#include "TextureBaker.h"

using namespace GEditor;

TextureBaker::TextureBaker(const Gleam::TextureDescriptor& descriptor)
	: mDescriptor(descriptor)
{
	
}

Gleam::Asset TextureBaker::Bake(const Gleam::Filesystem::path& directory) const
{
    auto guid = Gleam::Guid::NewGuid();
    auto typeGuid = Gleam::Reflection::GetClass<Gleam::TextureDescriptor>().Guid();
    Gleam::Asset asset(guid, typeGuid);
    // TODO:
    return asset;
}

const Gleam::TString& TextureBaker::Filename() const
{
	return mDescriptor.name;
}
