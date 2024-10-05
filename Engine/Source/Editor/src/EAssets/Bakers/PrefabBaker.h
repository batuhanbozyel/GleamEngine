#pragma once
#include "EAssets/AssetBaker.h"

namespace GEditor {

class PrefabBaker final : public AssetBaker
{
public:

	PrefabBaker(const Gleam::RefCounted<Gleam::World>& world);

	virtual void Bake(Gleam::FileStream& stream) const override;
    
    virtual Gleam::TString Filename() const override;
    
    virtual Gleam::Guid TypeGuid() const override;

private:

	Gleam::RefCounted<Gleam::World> mWorld;

};

} // namespace GEditor
