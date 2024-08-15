#include "MeshBaker.h"

using namespace GEditor;

MeshBaker::MeshBaker(const Gleam::MeshDescriptor& descriptor)
	: mDescriptor(descriptor)
{
	
}

Gleam::AssetReference MeshBaker::Bake(const Gleam::Filesystem::Path& directory) const
{
    auto guid = Gleam::Guid::NewGuid();
    const auto& typeGuid = Gleam::Reflection::GetClass<Gleam::MeshDescriptor>().Guid();
    Gleam::AssetReference asset{ .type = typeGuid, .guid = guid };

	auto filename = guid.ToString() + Gleam::Asset::extension().data();
    auto file = Gleam::File::Create(directory/filename, Gleam::FileType::Text);
    auto serializer = Gleam::JSONSerializer(file.GetStream());
    serializer.Serialize(mDescriptor);
    return asset;
}

Gleam::TString MeshBaker::Filename() const
{
    return mDescriptor.name + Gleam::Asset::extension().data();
}
