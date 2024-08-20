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
    auto file = Gleam::Filesystem::Create(directory/filename, Gleam::FileType::Text);
	auto& accessor = Gleam::Filesystem::Accessor(directory/filename);
	{
		auto serializer = Gleam::JSONSerializer(file.GetStream());

		std::lock_guard<std::mutex> lock(accessor.mutex);
		accessor.status = Gleam::FileStatus::Writing;
		serializer.Serialize(mDescriptor);
		accessor.status = Gleam::FileStatus::Available;
	}
	accessor.condition.notify_all();
    return asset;
}

Gleam::TString MeshBaker::Filename() const
{
    return mDescriptor.name + Gleam::Asset::extension().data();
}
