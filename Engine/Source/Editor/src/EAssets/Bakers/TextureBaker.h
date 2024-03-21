#pragma once
#include "EAssets/AssetBaker.h"

namespace GEditor {

class TextureBaker final : public AssetBaker
{
public:

	TextureBaker(const Gleam::TextureDescriptor& descriptor);

	virtual Gleam::Asset Bake(const Gleam::Filesystem::path& directory) const override;
    
    virtual const Gleam::TString& Filename() const override;

private:

	Gleam::TextureDescriptor mDescriptor;

};

} // namespace GEditor
