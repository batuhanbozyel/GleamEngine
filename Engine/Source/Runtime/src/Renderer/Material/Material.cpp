//
//  Material.cpp
//  GleamEngine
//
//  Created by Batuhan Bozyel on 3.04.2023.
//

#include "gpch.h"
#include "Material.h"
#include "Core/Application.h"
#include "Renderer/RenderSystem.h"

using namespace Gleam;

Material::Material(const MaterialDescriptor& descriptor)
    : IMaterial(descriptor.properties), mRenderQueue(descriptor.renderQueue)
{
    static auto renderSystem = GameInstance->GetSubsystem<RenderSystem>();
    
    mPasses.reserve(descriptor.passes.size());
    for (uint32_t i = 0; i < mPasses.size(); i++)
    {
        auto& pass = mPasses[i];
        auto& passDesc = descriptor.passes[i];

        pass.pipelineState = passDesc.pipelineState;
        pass.vertexFunction = renderSystem->GetDevice()->CreateShader(passDesc.vertexEntry, ShaderStage::Vertex);
        pass.fragmentFunction = renderSystem->GetDevice()->CreateShader(passDesc.fragmentEntry, ShaderStage::Fragment);
    }
    // TODO: Allocate GPU buffer
}

RefCounted<MaterialInstance> Material::CreateInstance()
{
    // TODO: Suballocate material instance data from material buffer
    return CreateRef<MaterialInstance>(shared_from_this(), mInstanceCount++);
}

const TArray<MaterialPass>& Material::GetPasses() const
{
    return mPasses;
}

RenderQueue Material::GetRenderQueue() const
{
    return mRenderQueue;
}
