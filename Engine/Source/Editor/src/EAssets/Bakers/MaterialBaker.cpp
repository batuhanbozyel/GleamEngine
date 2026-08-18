#include "MaterialBaker.h"
#include "EAssets/AssetRegistry.h"
#include "EAssets/AssetWriter.h"

#include "Assets/Asset.h"

using namespace GEditor;

// MaterialBaker
MaterialBaker::MaterialBaker(const Gleam::MaterialDescriptor& descriptor)
	: mDescriptor(descriptor)
{

}

void MaterialBaker::Bake(const Gleam::Path& directory, const AssetItem& item) const
{
	BinaryAssetWriter writer;
	writer.Write(directory, item, mDescriptor);
}

Gleam::TString MaterialBaker::Name() const
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
	BinaryAssetWriter writer;
	writer.Write(directory, item, mDescriptor);
}

Gleam::TString MaterialInstanceBaker::Name() const
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