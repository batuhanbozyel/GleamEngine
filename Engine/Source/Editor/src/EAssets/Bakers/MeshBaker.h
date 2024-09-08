#pragma once
#include "EAssets/AssetBaker.h"

namespace GEditor {

class MeshBaker final : public AssetBaker
{
public:

	MeshBaker(const Gleam::MeshDescriptor& descriptor);

	virtual Gleam::AssetReference Bake(const Gleam::Filesystem::Path& directory) const override;
    
    virtual Gleam::TString Filename() const override;
    
    virtual Gleam::Guid TypeGuid() const override;

private:

	Gleam::MeshDescriptor mDescriptor;

};

} // namespace GEditor
