//
//  Asset.h
//  Runtime
//
//  Created by Batuhan Bozyel on 5.10.2023.
//

#pragma once
#include "Core/GUID.h"

namespace Gleam {

class Asset final
{
public:
    
    static constexpr TStringView extension()
    {
        return ".asset";
    }
    
    Asset() = default;
    Asset(const Guid& guid, const Guid& type)
        : mGuid(guid), mType(type)
    {
        
    }

	~Asset() = default;
    
    bool operator==(const Asset &other) const
    {
        return mGuid == other.mGuid;
    }
    
    bool operator!=(const Asset& other) const
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
struct std::hash<Gleam::Asset>
{
    size_t operator()(const Gleam::Asset& asset) const
    {
        size_t hash = 0;
        Gleam::hash_combine(hash, asset.GetGuid());
        return hash;
    }
};
