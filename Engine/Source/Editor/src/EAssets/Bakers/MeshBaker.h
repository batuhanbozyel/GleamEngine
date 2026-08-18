#pragma once
#include "EAssets/AssetBaker.h"
#include "EAssets/Tools/MeshTools.h"
#include "Renderer/MeshDescriptor.h"

namespace GEditor {

class MeshBaker final : public AssetBaker
{
public:

	MeshBaker(MeshLodData&& lod);

	virtual void Bake(const Gleam::Path& directory, const AssetItem& item) const override;

	virtual Gleam::TString Name() const override;

	virtual Gleam::Guid TypeGuid() const override;

private:

	Gleam::TArray<MeshLodData> mLods;

};

} // namespace GEditor
