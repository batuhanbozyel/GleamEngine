//
//  Asset.h
//  Runtime
//
//  Created by Batuhan Bozyel on 5.10.2023.
//

#pragma once
#include "Core/GUID.h"

namespace Gleam {

class Asset
{
public:

	virtual ~Asset() = default;
    
private:
    
    GUID mGuid = InvalidGuid;
    
};

} // Gleam
