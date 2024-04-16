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
    
    auto serialized = Gleam::JSONSerializer::Serialize<Gleam::MeshDescriptor>(mDescriptor);
    auto file = Gleam::File(directory/Filename(), Gleam::FileType::Text);
    file.Write(serialized);
    return asset;
}

Gleam::TString MeshBaker::Filename() const
{
    return mDescriptor.name + Gleam::Asset::extension().data();
}
