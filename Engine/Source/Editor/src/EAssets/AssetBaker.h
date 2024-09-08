#pragma once
#include "Gleam.h"

namespace GEditor {

class AssetBaker
{
public:

	AssetBaker()
		: mGuid(Gleam::Guid::NewGuid())
	{

	}
    
	virtual ~AssetBaker() = default;

	virtual Gleam::AssetReference Bake(const Gleam::Filesystem::Path& directory) const = 0;

    virtual Gleam::TString Filename() const = 0;
    
    virtual Gleam::Guid TypeGuid() const = 0;

	const Gleam::Guid& GetGuid() const
	{
		return mGuid;
	}

private:

	Gleam::Guid mGuid;

};

} // namespace GEditor
