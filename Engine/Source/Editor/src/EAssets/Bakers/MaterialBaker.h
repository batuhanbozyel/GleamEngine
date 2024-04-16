#pragma once
#include "EAssets/AssetBaker.h"

namespace GEditor {

class MaterialBaker final : public AssetBaker
{
public:

	MaterialBaker(const Gleam::MaterialDescriptor& descriptor);

	virtual Gleam::Asset Bake(const Gleam::Filesystem::path& directory) const override;
    
    virtual Gleam::TString Filename() const override;

private:

	Gleam::MaterialDescriptor mDescriptor;

};

} // namespace GEditor
