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

	Asset(const AssetReference& reference, const TString& name)
		: mReference(reference)
		, mName(name)
	{

	}

	virtual ~Asset() = default;

	uint32_t ReferenceCount() const
	{
		return mRefCount;
	}

	const AssetReference& GetReference() const
	{
		return mReference;
	}

	const TString& GetName() const
	{
		return mName;
	}

    static constexpr TWStringView Extension()
    {
        return L".asset";
    }

private:

	AssetReference mReference;

	TString mName;

	uint32_t mRefCount = 0;
};

} // Gleam
