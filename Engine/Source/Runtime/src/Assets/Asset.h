//
//  Asset.h
//  Runtime
//
//  Created by Batuhan Bozyel on 5.10.2023.
//

#pragma once
#include "Core/GUID.h"

namespace Gleam {

struct Asset
{
    Filesystem::path path;
    
    static constexpr TStringView extension()
    {
        return ".asset";
    }
};

} // Gleam
