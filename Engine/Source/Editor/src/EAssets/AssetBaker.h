#pragma once
#include "Gleam.h"

namespace GEditor {

class AssetBaker
{
public:
    
	virtual ~AssetBaker() = default;

	virtual Gleam::AssetReference Bake(const Gleam::Filesystem::path& directory) const = 0;

    virtual Gleam::TString Filename() const = 0;

};

} // namespace GEditor
