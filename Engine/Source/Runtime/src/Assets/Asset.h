//
//  Asset.h
//  Runtime
//
//  Created by Batuhan Bozyel on 5.10.2023.
//

#pragma once
#include "AssetHeader.h"
#include "AssetReference.h"

#include "Container/Hash.h"

namespace Gleam {

class AssetManager;

class Asset
{
	friend class AssetManager;
public:

	Asset(const AssetReference& reference, const AssetHeader& header);

	virtual ~Asset() = default;

	uint32_t ReferenceCount() const;

	const AssetReference& GetReference() const;

	const AssetHeader& GetHeader() const;

	const TString& GetName() const;

	const AssetBlobDescriptor* FindBlob(const AssetBlobType& type, uint32_t slot, AssetPlatform platform, AssetBackend backend) const;

	template<typename T>
	const AssetBlobDescriptor* FindBlob(uint32_t slot, AssetPlatform platform, AssetBackend backend) const
	{
		return FindBlob(AssetUtils::BlobType<T>(), slot, platform, backend);
	}

	BufferRange GetBlobRange(const AssetBlobDescriptor& blob) const;

    static constexpr TWStringView Extension()
    {
        return L".asset";
    }

private:

	using AssetBlobVariants = TArray<uint32_t>;

	void BuildBlobTable();

	const AssetBlobDescriptor* ResolveBlob(const AssetBlobType& type, uint32_t slot, AssetPlatform platform, AssetBackend backend) const;

	AssetReference mReference;

	AssetHeader mHeader;

	HashMap<Guid, TArray<AssetBlobVariants>> mBlobTable;

	uint32_t mRefCount = 0;
};

} // Gleam
