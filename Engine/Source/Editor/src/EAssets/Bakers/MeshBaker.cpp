#include "MeshBaker.h"

using namespace GEditor;

MeshBaker::MeshBaker(const Gleam::MeshDescriptor& descriptor)
	: mDescriptor(descriptor)
{
	
}

Gleam::Asset MeshBaker::Bake(const Gleam::Filesystem::path& directory) const
{
    Gleam::Guid guid = Gleam::Guid::NewGuid();
    Gleam::Asset asset(guid);
    // TODO: 
    return asset;
}

const Gleam::TString& MeshBaker::Filename() const
{
    return mDescriptor.name;
}