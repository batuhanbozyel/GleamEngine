#pragma once
#include "EAssets/AssetBaker.h"
#include "Container/Pointer.h"

namespace Gleam {
class World;
} // namespace Gleam

namespace GEditor {

class PrefabBaker final : public AssetBaker
{
public:

	PrefabBaker(const Gleam::RefCounted<Gleam::World>& world);

	virtual void Bake(const Gleam::Path& directory, const AssetItem& item) const override;
    
    virtual Gleam::TString Name() const override;
    
    virtual Gleam::Guid TypeGuid() const override;

private:

	Gleam::RefCounted<Gleam::World> mWorld;

};

} // namespace GEditor
