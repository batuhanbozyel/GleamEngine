#pragma once
#include "EAssets/AssetBaker.h"

namespace GEditor {

class MaterialBaker final : public AssetBaker
{
public:

	MaterialBaker(const Gleam::MaterialDescriptor& descriptor);

	virtual void Bake(Gleam::FileStream& stream) const override;
    
    virtual Gleam::TString Filename() const override;
    
    virtual Gleam::Guid TypeGuid() const override;

	const Gleam::MaterialDescriptor& GetDescriptor() const;

private:

	Gleam::MaterialDescriptor mDescriptor;

};

class MaterialInstanceBaker final : public AssetBaker
{
public:

	MaterialInstanceBaker(const Gleam::MaterialInstanceDescriptor& descriptor);

	virtual void Bake(Gleam::FileStream& stream) const override;

	virtual Gleam::TString Filename() const override;
    
    virtual Gleam::Guid TypeGuid() const override;

	const Gleam::MaterialInstanceDescriptor& GetDescriptor() const;

private:

	Gleam::MaterialInstanceDescriptor mDescriptor;

};

} // namespace GEditor
