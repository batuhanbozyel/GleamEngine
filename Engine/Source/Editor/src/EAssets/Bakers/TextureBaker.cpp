#include "TextureBaker.h"

using namespace GEditor;

TextureBaker::TextureBaker(const Gleam::TextureDescriptor& descriptor)
	: mDescriptor(descriptor)
{
	
}

Gleam::AssetReference TextureBaker::Bake(const Gleam::Filesystem::Path& directory) const
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

Gleam::TString TextureBaker::Filename() const
{
	return mDescriptor.name;
}
