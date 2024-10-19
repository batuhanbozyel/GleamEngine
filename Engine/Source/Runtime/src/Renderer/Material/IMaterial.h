//
//  IMaterial.h
//  GleamEngine
//
//  Created by Batuhan Bozyel on 3.04.2023.
//

#pragma once
#include "MaterialProperty.h"
#include "Assets/Asset.h"

namespace Gleam {

class IMaterial : public Asset
{
public:

    IMaterial(const TArray<MaterialProperty>& properties)
        : mProperties(properties)
    {

    }

    const TArray<MaterialProperty>& GetProperties() const
    {
        return mProperties;
    }

    uint32_t GetPropertyIndex(const TString& name) const
    {
        for (uint32_t i = 0; i < mProperties.size(); i++)
        {
            if (mProperties[i].name == name) { return i; }
        }
        return ~0u;
    }
    
protected:

    TArray<MaterialProperty> mProperties;
    
};

} // namespace Gleam
