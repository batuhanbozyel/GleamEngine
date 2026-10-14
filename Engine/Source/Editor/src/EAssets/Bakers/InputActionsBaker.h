#pragma once
#include "EAssets/AssetBaker.h"
#include "Input/InputActionDescriptor.h"

namespace GEditor {

class InputActionsBaker final : public AssetBaker
{
public:

	InputActionsBaker(const Gleam::InputActionAssetDescriptor& descriptor);

	virtual void Bake(const Gleam::Path& directory, const AssetItem& item) const override;

	virtual Gleam::TString Name() const override;

	virtual Gleam::Guid TypeGuid() const override;

	const Gleam::InputActionAssetDescriptor& GetDescriptor() const;

private:

	Gleam::InputActionAssetDescriptor mDescriptor;

};

} // namespace GEditor
