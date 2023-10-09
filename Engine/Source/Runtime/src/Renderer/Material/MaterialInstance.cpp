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

MaterialInstance::MaterialInstance(const RefCounted<IMaterial>& baseMaterial, uint32_t uniqueId)
	: IMaterial(baseMaterial->GetProperties()), mBaseMaterial(baseMaterial), mPropertyValues(baseMaterial->GetProperties().size()), mUniqueId(uniqueId)
{
    
}

void MaterialInstance::SetProperty(const TString& name, const MaterialPropertyValue& value)
{
	mPropertyValues[GetPropertyIndex(name)] = value;
}

const RefCounted<IMaterial>& MaterialInstance::GetBaseMaterial() const
{
    return mBaseMaterial;
}

uint32_t MaterialInstance::GetUniqueId() const
{
    return mUniqueId;
}
