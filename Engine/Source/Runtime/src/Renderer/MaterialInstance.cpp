//
//  MaterialInstance.cpp
//  GleamEngine
//
//  Created by Batuhan Bozyel on 9.04.2023.
//

#include "gpch.h"
#include "Material.h"
#include "MaterialInstance.h"

using namespace Gleam;

MaterialInstance::MaterialInstance(const RefCounted<Material>& material, uint32_t uniqueId)
	: mMaterial(material), mPropertyValues(material->GetProperties().size()), mUniqueId(uniqueId)
{
    
}

void MaterialInstance::SetProperty(const TString& name, const MaterialPropertyValue& value)
{
    mPropertyValues[mMaterial->GetPropertyIndex(name)] = value;
}

const RefCounted<Material>& MaterialInstance::GetMaterial() const
{
    return mMaterial;
}

uint32_t MaterialInstance::GetUniqueId() const
{
    return mUniqueId;
}
