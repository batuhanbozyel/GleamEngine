#pragma once
#include "EAssets/AssetBaker.h"

namespace GEditor {

class TextureBaker final : public AssetBaker
{
public:

	TextureBaker(const Gleam::TextureDescriptor& descriptor);

	virtual void Bake(Gleam::FileStream& stream) const override;
    
    virtual Gleam::TString Filename() const override;
    
    virtual Gleam::Guid TypeGuid() const override;

private:

	Gleam::TextureDescriptor mDescriptor;

};

} // namespace GEditor
