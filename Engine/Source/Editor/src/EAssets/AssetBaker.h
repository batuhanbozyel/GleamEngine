#pragma once
#include "Core/GUID.h"
#include "IO/Path.h"

namespace GEditor {

struct AssetItem;

class AssetBaker
{
public:

	virtual ~AssetBaker() = default;

	virtual void Bake(const Gleam::Path& directory, const AssetItem& item) const = 0;

    virtual Gleam::TString Filename() const = 0;
    
    virtual Gleam::Guid TypeGuid() const = 0;

};

} // namespace GEditor
