#pragma once
#include "EAssets/AssetBaker.h"

namespace GEditor {

class PrefabBaker final : public AssetBaker
{
public:

	PrefabBaker(Gleam::World&& world);

	virtual void Bake(Gleam::FileStream& stream) const override;
    
    virtual Gleam::TString Filename() const override;
    
    virtual Gleam::Guid TypeGuid() const override;

private:

	Gleam::World mWorld;

};

} // namespace GEditor
