//
//  AssetReference.h
//  Runtime
//
//  Created by Batuhan Bozyel on 5.10.2023.
//

#pragma once
#include "Core/GUID.h"
#include "Reflection/Macro.h"

namespace Gleam {

GSTRUCT(AssetReference, "7C59A995-BA05-4A2A-9F48-AD5170F05CF8", Serializable)
{
	GFIELD("E09BE041-51C4-4848-ACD9-50C1D8ED1872", Serializable)
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
