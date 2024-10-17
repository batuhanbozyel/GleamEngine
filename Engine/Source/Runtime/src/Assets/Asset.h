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
	virtual ~Asset() = default;
	
    static constexpr TStringView Extension()
    {
        return ".asset";
    }
};

} // Gleam
