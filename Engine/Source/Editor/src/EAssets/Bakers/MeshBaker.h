#pragma once
#include "EAssets/AssetBaker.h"

namespace GEditor {

class MeshBaker final : public AssetBaker
{
public:

	MeshBaker(const Gleam::MeshDescriptor& descriptor);

	virtual void Bake(Gleam::FileStream& stream) const override;
    
    virtual Gleam::TString Filename() const override;
    
    virtual Gleam::Guid TypeGuid() const override;

	const Gleam::MeshDescriptor& GetDescriptor() const;

private:

	Gleam::MeshDescriptor mDescriptor;

};

} // namespace GEditor
