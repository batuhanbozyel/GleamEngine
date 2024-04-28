#include "MaterialBaker.h"

using namespace GEditor;

MaterialBaker::MaterialBaker(const Gleam::MaterialDescriptor& descriptor)
	: mDescriptor(descriptor)
{
	
}

Gleam::AssetReference MaterialBaker::Bake(const Gleam::Filesystem::path& directory) const
{
    const auto& typeGuid = Gleam::Reflection::GetClass<Gleam::MaterialDescriptor>().Guid();
    auto guid = Gleam::Guid::NewGuid();
    Gleam::AssetReference asset{ .type = typeGuid, .guid = guid };
    // TODO:
    return asset;
}

Gleam::TString MaterialBaker::Filename() const
{
    return mDescriptor.name + Gleam::Asset::extension().data();
}
