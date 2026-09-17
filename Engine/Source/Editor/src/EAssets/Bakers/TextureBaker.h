#pragma once
#include "EAssets/AssetBaker.h"
#include "EAssets/Tools/TextureTools.h"
#include "Renderer/TextureDescriptor.h"

namespace GEditor {

class TextureBaker final : public AssetBaker
{
public:

	TextureBaker(TextureData&& textureData);

	virtual void Bake(const Gleam::Path& directory, const AssetItem& item) const override;

    virtual Gleam::TString Name() const override;

    virtual Gleam::Guid TypeGuid() const override;

private:

	TextureData mTextureData;

};

} // namespace GEditor
