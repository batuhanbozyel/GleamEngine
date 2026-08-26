//
//  Asset.h
//  Runtime
//
//  Created by Batuhan Bozyel on 5.10.2023.
//

#pragma once
#include "AssetHeader.h"
#include "AssetReference.h"

namespace Gleam {

class AssetManager;

class Asset
{
	friend class AssetManager;
public:

	Asset(const AssetReference& reference, const AssetHeader& header)
		: mReference(reference)
		, mHeader(header)
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

	const AssetHeader& GetHeader() const
	{
		return mHeader;
	}

	const TString& GetName() const
	{
		return mHeader.name;
	}

    static constexpr TWStringView Extension()
    {
        return L".asset";
    }

private:

	AssetReference mReference;

	AssetHeader mHeader;

	uint32_t mRefCount = 0;
};

} // Gleam
