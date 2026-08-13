//
//  MaterialInstance.cpp
//  GleamEngine
//
//  Created by Batuhan Bozyel on 9.04.2023.
//

#include "gpch.h"
#include "MaterialInstance.h"
#include "Material.h"

#include "Core/Globals.h"
#include "Core/Application.h"
#include "Assets/AssetManager.h"

using namespace Gleam;

MaterialInstance::MaterialInstance(const AssetReference& reference, const MaterialInstanceDescriptor& descriptor)
    : IMaterial(reference, descriptor.name, descriptor.properties)
	, mBaseMaterial(descriptor.material)
{
	mPropertyValues.reserve(descriptor.properties.size());
	for (const auto& property : descriptor.properties)
	{
		mPropertyValues.push_back(property.value);
	}

	auto assetManager = Globals::GameInstance->GetSubsystem<AssetManager>();
	auto material = assetManager->Load<Material>(mBaseMaterial);
	mResourceView = material->CreateInstance(mPropertyValues);
}

MaterialInstance::~MaterialInstance()
{
	auto assetManager = Globals::GameInstance->GetSubsystem<AssetManager>();
	for (uint32_t i = 0; i < mProperties.size(); ++i)
	{
		if (mProperties[i].type == MaterialPropertyType::Texture2D)
		{
			assetManager->Release(mPropertyValues[i].texture);
		}
	}

	auto material = assetManager->Get<Material>(mBaseMaterial);
	material->DestroyInstance(mResourceView);
	assetManager->Release(mBaseMaterial);
}

void MaterialInstance::SetProperty(const TString& name, const MaterialPropertyValue& value)
{
	auto propertyIdx = GetPropertyIndex(name);
	if (propertyIdx != ~0u)
	{
		mPropertyValues[propertyIdx] = value;
	}
}

const AssetReference& MaterialInstance::GetBaseMaterial() const
{
    return mBaseMaterial;
}

uint32_t MaterialInstance::GetID() const
{
    return mResourceView.data;
}
