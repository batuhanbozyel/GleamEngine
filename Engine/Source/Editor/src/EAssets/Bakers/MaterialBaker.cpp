#include "MaterialBaker.h"

using namespace GEditor;

MaterialBaker::MaterialBaker(const Gleam::MaterialDescriptor& descriptor)
	: mDescriptor(descriptor)
{
	
}

Gleam::AssetReference MaterialBaker::Bake(const Gleam::Filesystem::Path& directory) const
{
    auto guid = Gleam::Guid::NewGuid();
    Gleam::AssetReference asset{ .guid = guid };
    // TODO:
    return asset;
}

Gleam::TString MaterialBaker::Filename() const
{
    return mDescriptor.name;
}
