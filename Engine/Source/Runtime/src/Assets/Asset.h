//
//  Asset.h
//  Runtime
//
//  Created by Batuhan Bozyel on 5.10.2023.
//

#pragma once
#include "AssetReference.h"

namespace Gleam {

class AssetManager;

class Asset
{
	friend class AssetManager;
public:

	virtual ~Asset() = default;

	uint32_t ReferenceCount() const
	{
		return mRefCount;
	}

    static constexpr TWStringView Extension()
    {
        return L".asset";
    }

private:

	uint32_t mRefCount = 0;
};

} // Gleam
