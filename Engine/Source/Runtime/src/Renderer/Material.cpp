//
//  Material.cpp
//  GleamEngine
//
//  Created by Batuhan Bozyel on 3.04.2023.
//

#include "gpch.h"
#include "Material.h"
#include "MaterialInstance.h"

using namespace Gleam;

Material::Material(const MaterialDescriptor& descriptor)
    : mDescriptor(descriptor)
{
    // TODO: Allocate GPU buffer
}

RefCounted<MaterialInstance> Material::CreateInstance()
{
    // TODO: Suballocate material instance data from material buffer
    return CreateRef<MaterialInstance>(shared_from_this(), mInstanceCount++);
}

const TArray<MaterialPass>& Material::GetPasses() const
{
    return mDescriptor.passes;
}

const TArray<MaterialProperty>& Material::GetProperties() const
{
    return mDescriptor.properties;
}

RenderQueue Material::GetRenderQueue() const
{
    return mDescriptor.renderQueue;
}

uint32_t Material::GetPropertyIndex(const TString& name) const
{
    for (uint32_t i = 0; i < mDescriptor.properties.size(); i++)
    {
        if (mDescriptor.properties[i].name == name)
        {
            return i;
        }
    }
    return 0;
}
