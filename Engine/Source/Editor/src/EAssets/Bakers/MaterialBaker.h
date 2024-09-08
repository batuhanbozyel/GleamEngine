#pragma once
#include "EAssets/AssetBaker.h"

namespace GEditor {

class MaterialBaker final : public AssetBaker
{
public:

	MaterialBaker(const Gleam::MaterialDescriptor& descriptor);

	virtual Gleam::AssetReference Bake(const Gleam::Filesystem::Path& directory) const override;
    
    virtual Gleam::TString Filename() const override;

private:

	Gleam::MaterialDescriptor mDescriptor;

};

class MaterialInstanceBaker final : public AssetBaker
{
public:

	MaterialInstanceBaker(const Gleam::MaterialInstanceDescriptor& descriptor);

	virtual Gleam::AssetReference Bake(const Gleam::Filesystem::Path& directory) const override;

	virtual Gleam::TString Filename() const override;

private:

	Gleam::MaterialInstanceDescriptor mDescriptor;

};

} // namespace GEditor
