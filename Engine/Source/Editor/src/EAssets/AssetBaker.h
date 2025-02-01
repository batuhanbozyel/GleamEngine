#pragma once
#include "Gleam.h"

namespace GEditor {

struct AssetItem;

class AssetBaker
{
public:

	virtual ~AssetBaker() = default;

	virtual void Bake(const Gleam::Filesystem::Path& directory, const AssetItem& item) const = 0;

    virtual Gleam::TString Filename() const = 0;
    
    virtual Gleam::Guid TypeGuid() const = 0;

};

} // namespace GEditor
