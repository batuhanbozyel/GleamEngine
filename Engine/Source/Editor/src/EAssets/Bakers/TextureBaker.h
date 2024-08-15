#pragma once
#include "EAssets/AssetBaker.h"

namespace GEditor {

class TextureBaker final : public AssetBaker
{
public:

	TextureBaker(const Gleam::TextureDescriptor& descriptor);

	virtual Gleam::AssetReference Bake(const Gleam::Filesystem::Path& directory) const override;
    
    virtual Gleam::TString Filename() const override;

private:

	Gleam::TextureDescriptor mDescriptor;

};

} // namespace GEditor
