#include "TextureBaker.h"

using namespace GEditor;

TextureBaker::TextureBaker(const Gleam::TextureDescriptor& descriptor)
	: mDescriptor(descriptor)
{
	
}

Gleam::Asset TextureBaker::Bake(const Gleam::Filesystem::path& directory) const
{
    Gleam::Guid guid = Gleam::Guid::NewGuid();
    Gleam::Asset asset(guid);
    // TODO:
    return asset;
}

const Gleam::TString& TextureBaker::Filename() const
{
	return mDescriptor.name;
}