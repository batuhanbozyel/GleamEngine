#include "gpch.h"
#include "InputActionAsset.h"

using namespace Gleam;

InputActionAsset::InputActionAsset(const AssetReference& reference, const AssetHeader& header, const InputActionAssetDescriptor& descriptor)
	: Asset(reference, header), mDescriptor(descriptor)
{

}

const InputActionAssetDescriptor& InputActionAsset::GetDescriptor() const
{
	return mDescriptor;
}

const InputActionMapDescriptor* InputActionAsset::FindActionMap(TStringView name) const
{
	for (const auto& map : mDescriptor.maps)
	{
		if (map.name == name)
		{
			return &map;
		}
	}
	return nullptr;
}
