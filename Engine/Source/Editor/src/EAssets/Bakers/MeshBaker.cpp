#include "MeshBaker.h"
#include "EAssets/AssetRegistry.h"

#include "Gleam.h"

using namespace GEditor;

MeshBaker::MeshBaker(const Gleam::MeshDescriptor& descriptor)
	: mDescriptor(descriptor)
{
	
}

void MeshBaker::Bake(const Gleam::Path& directory, const AssetItem& item) const
{
	auto filename = Gleam::TWString(item.reference.guid.ToString()) + Gleam::Asset::Extension();
	auto file = Gleam::Filesystem::Create(directory / filename, Gleam::FileType::Binary);
	auto accessor = Gleam::Filesystem::WriteAccessor(directory / filename);

	auto serializer = Gleam::BinarySerializer();
	serializer.Serialize(mDescriptor, file.GetStream());
}

Gleam::TString MeshBaker::Filename() const
{
    return mDescriptor.name;
}

Gleam::Guid MeshBaker::TypeGuid() const
{
    return Gleam::Reflection::GetClass<decltype(mDescriptor)>().Guid();
}

const Gleam::MeshDescriptor& MeshBaker::GetDescriptor() const
{
	return mDescriptor;
}
