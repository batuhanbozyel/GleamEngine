//
//  AssetReference.h
//  Runtime
//
//  Created by Batuhan Bozyel on 5.10.2023.
//

#pragma once
#include "Core/GUID.h"

namespace Gleam {

class AssetReference final
{
public:
    
    AssetReference() = default;
    AssetReference(const Guid& guid, const Guid& type)
        : mGuid(guid), mType(type)
    {
        
    }

	~AssetReference() = default;
    
    bool operator==(const AssetReference &other) const
    {
        return mGuid == other.mGuid;
    }
    
    bool operator!=(const AssetReference& other) const
    {
        return !(*this == other);
    }

	const Guid& GetType() const
	{
		return mType;
	}

	const Guid& GetGuid() const
	{
		return mGuid;
	}
    
private:

	Guid mType = Guid::InvalidGuid();
    
    Guid mGuid = Guid::InvalidGuid();
    
};

} // Gleam

template <>
struct std::hash<Gleam::AssetReference>
{
    size_t operator()(const Gleam::AssetReference& asset) const
    {
        size_t hash = 0;
        Gleam::hash_combine(hash, asset.GetGuid());
        return hash;
    }
};
