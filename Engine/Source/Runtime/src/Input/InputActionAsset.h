#pragma once
#include "InputActionDescriptor.h"

#include "Assets/Asset.h"

namespace Gleam {

class InputActionAsset final : public Asset
{
public:

	InputActionAsset(const AssetReference& reference, const AssetHeader& header, const InputActionAssetDescriptor& descriptor);

	const InputActionAssetDescriptor& GetDescriptor() const;

	const InputActionMapDescriptor* FindActionMap(TStringView name) const;

private:

	InputActionAssetDescriptor mDescriptor;

};

} // namespace Gleam
