#pragma once
#include "Gleam.h"

namespace GEditor {

class AssetBaker
{
public:

	virtual ~AssetBaker() = default;

	virtual void Bake(Gleam::FileStream& stream) const = 0;

    virtual Gleam::TString Filename() const = 0;
    
    virtual Gleam::Guid TypeGuid() const = 0;

};

} // namespace GEditor
