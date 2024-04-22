//
//  AssetReference.h
//  Runtime
//
//  Created by Batuhan Bozyel on 5.10.2023.
//

#pragma once
#include "Core/GUID.h"

namespace Gleam {

struct AssetReference
{
	Guid type = Guid::InvalidGuid();
	Guid guid = Guid::InvalidGuid();
    
    bool operator==(const AssetReference &other) const
    {
        return guid == other.guid;
    }
    
    bool operator!=(const AssetReference& other) const
    {
        return !(*this == other);
    }
};

} // Gleam

template <>
struct std::hash<Gleam::AssetReference>
{
    size_t operator()(const Gleam::AssetReference& asset) const
    {
        size_t hash = 0;
        Gleam::hash_combine(hash, asset.guid);
        return hash;
    }
};
