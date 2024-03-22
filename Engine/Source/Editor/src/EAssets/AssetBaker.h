#pragma once
#include "Gleam.h"

namespace GEditor {

class AssetBaker
{
public:
    
	virtual ~AssetBaker() = default;

	virtual Gleam::Asset Bake(const Gleam::Filesystem::path& directory) const = 0;

    virtual const Gleam::TString& Filename() const = 0;

};

} // namespace GEditor
