//
//  Material.cpp
//  GleamEngine
//
//  Created by Batuhan Bozyel on 3.04.2023.
//

#include "gpch.h"
#include "Material.h"
#include "MaterialInstance.h"

#include "Core/Engine.h"
#include "Core/Globals.h"
#include "Core/Application.h"
#include "Assets/AssetManager.h"
#include "Renderer/Texture2D.h"
#include "Renderer/RenderSystem.h"

using namespace Gleam;

Material::Material(const MaterialDescriptor& descriptor)
    : IMaterial(descriptor.properties)
    , mName(descriptor.name)
	, mInstanceDescriptorHeap(MaxMaterialInstances)
{
    static auto renderSystem = Globals::Engine->GetSubsystem<RenderSystem>();
    // TODO: Allocate GPU buffer
}

void Material::Release()
{
	static auto renderSystem = Globals::Engine->GetSubsystem<RenderSystem>();
	auto device = renderSystem->GetDevice();

	device->AddPooledObject([buffer = mBuffer]() mutable
	{
		//device->Dispose(buffer);
	});
}

ShaderResourceIndex Material::CreateInstance(const TArray<MaterialPropertyValue>& values)
{
	GLEAM_ASSERT(values.size() == mProperties.size(), "Material properties do not match with instance properties.");

	auto assetManager = Globals::GameInstance->GetSubsystem<AssetManager>();
	for (uint32_t i = 0; i < values.size(); ++i)
	{
		if (mProperties[i].type == MaterialPropertyType::Texture2D)
		{
			const auto& asset = values[i].texture;
			if (asset.guid != Guid::InvalidGuid())
			{
				auto texture = assetManager->Load<Texture2D>(values[i].texture);
			}
		}
	}

	// TODO: update GPU buffer with instance values
	return mInstanceDescriptorHeap.Allocate();
}

const Buffer& Material::GetBuffer() const
{
    return mBuffer;
}

const TString& Material::GetName() const
{
    return mName;
}

uint32_t Material::GetPipelineHash() const
{
	return mPipelineStateHash;
}