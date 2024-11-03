//
//  Asset.h
//  Runtime
//
//  Created by Batuhan Bozyel on 5.10.2023.
//

#pragma once

namespace Gleam {

class Asset
{
public:
	virtual ~Asset() = default;

	virtual void Release() = 0;
	
    static constexpr TStringView Extension()
    {
        return ".asset";
    }
};

} // Gleam
