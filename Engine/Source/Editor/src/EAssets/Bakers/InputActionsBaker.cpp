#include "InputActionsBaker.h"
#include "EAssets/AssetRegistry.h"
#include "EAssets/AssetWriter.h"

#include "Assets/Asset.h"

using namespace GEditor;

InputActionsBaker::InputActionsBaker(const Gleam::InputActionAssetDescriptor& descriptor)
	: mDescriptor(descriptor)
{

}

void InputActionsBaker::Bake(const Gleam::Path& directory, const AssetItem& item) const
{
	BinaryAssetWriter writer;
	writer.Write(directory, item, mDescriptor);
}

Gleam::TString InputActionsBaker::Name() const
{
	return mDescriptor.name;
}

Gleam::Guid InputActionsBaker::TypeGuid() const
{
	return Gleam::Reflection::GetClass<decltype(mDescriptor)>().Guid();
}

const Gleam::InputActionAssetDescriptor& InputActionsBaker::GetDescriptor() const
{
	return mDescriptor;
}
