#include "MaterialBaker.h"

using namespace GEditor;

// MaterialBaker
MaterialBaker::MaterialBaker(const Gleam::MaterialDescriptor& descriptor)
	: mDescriptor(descriptor)
{
	
}

Gleam::AssetReference MaterialBaker::Bake(const Gleam::Filesystem::Path& directory) const
{
	const auto& guid = GetGuid();
	Gleam::AssetReference asset{ .guid = guid };

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

Gleam::TString MaterialBaker::Filename() const
{
    return mDescriptor.name;
}

Gleam::Guid MaterialBaker::TypeGuid() const
{
    return Gleam::Reflection::GetClass<decltype(mDescriptor)>().Guid();
}

// MaterialInstanceBaker
MaterialInstanceBaker::MaterialInstanceBaker(const Gleam::MaterialInstanceDescriptor& descriptor)
	: mDescriptor(descriptor)
{

}

Gleam::AssetReference MaterialInstanceBaker::Bake(const Gleam::Filesystem::Path& directory) const
{
	const auto& guid = GetGuid();
	Gleam::AssetReference asset{ .guid = guid };

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

Gleam::TString MaterialInstanceBaker::Filename() const
{
	return mDescriptor.name;
}

Gleam::Guid MaterialInstanceBaker::TypeGuid() const
{
    return Gleam::Reflection::GetClass<decltype(mDescriptor)>().Guid();
}
