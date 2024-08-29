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
    : IMaterial(descriptor.properties), mRenderQueue(descriptor.renderQueue)
{
    static auto renderSystem = Globals::Engine->GetSubsystem<RenderSystem>();
    // TODO: Allocate GPU buffer
}

RefCounted<MaterialInstance> Material::CreateInstance()
{
    // TODO: Suballocate material instance data from material buffer
    return CreateRef<MaterialInstance>(shared_from_this(), mInstanceCount++);
}

size_t Material::GetPipelineHash() const
{
	return mPipelineStateHash;
}

RenderQueue Material::GetRenderQueue() const
{
    return mRenderQueue;
}

const Shader& Material::GetShader() const
{
	return mShader;
}
