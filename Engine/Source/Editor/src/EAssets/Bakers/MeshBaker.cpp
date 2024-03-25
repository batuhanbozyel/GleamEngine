#include "MeshBaker.h"

using namespace GEditor;

MeshBaker::MeshBaker(const Gleam::MeshDescriptor& descriptor)
	: mDescriptor(descriptor)
{
	
}

Gleam::Asset MeshBaker::Bake(const Gleam::Filesystem::path& directory) const
{
    const auto& typeGuid = Gleam::Reflection::GetClass<Gleam::MeshDescriptor>().Guid();
    auto guid = Gleam::Guid::NewGuid();
    Gleam::Asset asset(guid, typeGuid);
    // TODO:
    return asset;
}

const Gleam::TString& MeshBaker::Filename() const
{
    return mDescriptor.name;
}
