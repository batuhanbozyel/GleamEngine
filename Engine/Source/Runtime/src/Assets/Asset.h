//
//  Asset.h
//  Runtime
//
//  Created by Batuhan Bozyel on 5.10.2023.
//

#pragma once

namespace Gleam {

struct Asset
{
    Filesystem::Path path;
    
    bool operator==(const Asset& other) const
    {
        return path == other.path;
    }
    
    bool operator!=(const Asset& other) const
    {
        return !(*this == other);
    }
    
    static constexpr TStringView Extension()
    {
        return ".asset";
    }
};

} // Gleam
