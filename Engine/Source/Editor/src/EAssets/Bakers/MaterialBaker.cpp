#include "MaterialBaker.h"

using namespace GEditor;

MaterialBaker::MaterialBaker(const Gleam::MaterialDescriptor& descriptor)
	: mDescriptor(descriptor)
{
	
}

Gleam::Asset MaterialBaker::Bake(const Gleam::Filesystem::path& directory) const
{
    auto guid = Gleam::Guid::NewGuid();
    auto typeGuid = Gleam::Reflection::GetClass<Gleam::MaterialDescriptor>().Guid();
    Gleam::Asset asset(guid, typeGuid);
    // TODO:
    return asset;
}

const Gleam::TString& MaterialBaker::Filename() const
{
    return mDescriptor.name;
}
