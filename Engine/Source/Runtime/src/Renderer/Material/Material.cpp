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
#include "Renderer/RenderSystem.h"

using namespace Gleam;

Material::Material(const MaterialDescriptor& descriptor)
    : IMaterial(descriptor.properties)
    , mName(descriptor.name)
{
    static auto renderSystem = Globals::Engine->GetSubsystem<RenderSystem>();
    // TODO: Allocate GPU buffer
}

void Material::Dispose()
{
	static auto renderSystem = Globals::Engine->GetSubsystem<RenderSystem>();
	auto device = renderSystem->GetDevice();

	device->AddPooledObject([&]()
	{
		//device->Dispose(mBuffer);
	});
}

uint32_t Material::CreateInstance(const TArray<MaterialPropertyValue>& values)
{
	// TODO: update GPU buffer with instance values
	return mInstanceCount++;
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