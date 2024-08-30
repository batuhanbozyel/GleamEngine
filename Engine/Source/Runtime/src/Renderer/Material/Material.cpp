//
//  Material.cpp
//  GleamEngine
//
//  Created by Batuhan Bozyel on 3.04.2023.
//

#include "gpch.h"
#include "Material.h"

#include "Core/Engine.h"
#include "Core/Globals.h"
#include "Renderer/RenderSystem.h"

using namespace Gleam;

Material::Material(const MaterialDescriptor& descriptor)
    : IMaterial(descriptor.properties)
{
    static auto renderSystem = Globals::Engine->GetSubsystem<RenderSystem>();
    // TODO: Allocate GPU buffer
}

RefCounted<MaterialInstance> Material::CreateInstance()
{
    // TODO: Suballocate material instance data from material buffer
    return CreateRef<MaterialInstance>(shared_from_this(), mInstanceCount++);
}

uint32_t Material::GetPipelineHash() const
{
	return mPipelineStateHash;
}

const Shader& Material::GetShader() const
{
	return mShader;
}
