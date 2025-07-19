#include "MaterialBaker.h"
#include "EAssets/AssetRegistry.h"

using namespace GEditor;

// MaterialBaker
MaterialBaker::MaterialBaker(const Gleam::MaterialDescriptor& descriptor)
	: mDescriptor(descriptor)
{
	
}

void MaterialBaker::Bake(const Gleam::Path& directory, const AssetItem& item) const
{
	auto filename = Gleam::TWString(item.reference.guid.ToString()) + Gleam::Asset::Extension();
	auto file = Gleam::Filesystem::Create(directory / filename, Gleam::FileType::Binary);
	auto accessor = Gleam::Filesystem::WriteAccessor(directory / filename);

	auto serializer = Gleam::BinarySerializer();
	serializer.Serialize(mDescriptor, file.GetStream());
}

Gleam::TString MaterialBaker::Filename() const
{
    return mDescriptor.name;
}

Gleam::Guid MaterialBaker::TypeGuid() const
{
    return Gleam::Reflection::GetClass<decltype(mDescriptor)>().Guid();
}

const Gleam::MaterialDescriptor& MaterialBaker::GetDescriptor() const
{
	return mDescriptor;
}

// MaterialInstanceBaker
MaterialInstanceBaker::MaterialInstanceBaker(const Gleam::MaterialInstanceDescriptor& descriptor)
	: mDescriptor(descriptor)
{

}

void MaterialInstanceBaker::Bake(const Gleam::Path& directory, const AssetItem& item) const
{
	auto filename = Gleam::TWString(item.reference.guid.ToString()) + Gleam::Asset::Extension();
	auto file = Gleam::Filesystem::Create(directory / filename, Gleam::FileType::Binary);
	auto accessor = Gleam::Filesystem::WriteAccessor(directory / filename);

	auto serializer = Gleam::BinarySerializer();
	serializer.Serialize(mDescriptor, file.GetStream());
}

Gleam::TString MaterialInstanceBaker::Filename() const
{
	return mDescriptor.name;
}

Gleam::Guid MaterialInstanceBaker::TypeGuid() const
{
    return Gleam::Reflection::GetClass<decltype(mDescriptor)>().Guid();
}

const Gleam::MaterialInstanceDescriptor& MaterialInstanceBaker::GetDescriptor() const
{
	return mDescriptor;
}