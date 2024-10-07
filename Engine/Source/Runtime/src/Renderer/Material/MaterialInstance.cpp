//
//  MaterialInstance.cpp
//  GleamEngine
//
//  Created by Batuhan Bozyel on 9.04.2023.
//

#include "gpch.h"
#include "MaterialInstance.h"
#include "MaterialSystem.h"

#include "Core/Globals.h"
#include "Core/Application.h"

using namespace Gleam;

MaterialInstance::MaterialInstance(const MaterialInstanceDescriptor& descriptor)
    : IMaterial(descriptor.properties)
	, mBaseMaterial(descriptor.material)
{
	mPropertyValues.reserve(descriptor.properties.size());
	for (const auto& property : descriptor.properties)
	{
		mPropertyValues.push_back(property.value);
	}

	auto materialSystem = Globals::GameInstance->GetSubsystem<MaterialSystem>();
	auto& material = materialSystem->GetMaterial(mBaseMaterial);
	mUniqueId = material.CreateInstance(mPropertyValues);
}

void MaterialInstance::SetProperty(const TString& name, const MaterialPropertyValue& value)
{
	mPropertyValues[GetPropertyIndex(name)] = value;
}

const AssetReference& MaterialInstance::GetBaseMaterial() const
{
    return mBaseMaterial;
}

uint32_t MaterialInstance::GetUniqueId() const
{
    return mUniqueId;
}
