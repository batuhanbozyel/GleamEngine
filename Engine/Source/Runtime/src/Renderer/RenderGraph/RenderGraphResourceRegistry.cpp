//
//  RenderGraphResourceRegistry.cpp
//  Runtime
//
//  Created by Batuhan Bozyel on 22.04.2023.
//

#include "gpch.h"
#include "RenderGraphResourceRegistry.h"

using namespace Gleam;

void RenderGraphResourceRegistry::Clear()
{
    mNodes.clear();
    mEntries.clear();
}

NO_DISCARD RenderTextureHandle RenderGraphResourceRegistry::CreateRT(const TextureDescriptor& descriptor)
{
    RenderTextureHandle resource(static_cast<uint32_t>(mEntries.size()));
    mEntries.emplace_back(RenderGraphResourceEntry(descriptor, resource));
    
    RenderTextureHandle node(static_cast<uint32_t>(mNodes.size()));
    mNodes.emplace_back(node, resource);
    return node;
}

NO_DISCARD BufferHandle RenderGraphResourceRegistry::CreateBuffer(const BufferDescriptor& descriptor)
{
    BufferHandle resource(static_cast<uint32_t>(mEntries.size()));
    mEntries.emplace_back(RenderGraphResourceEntry(descriptor, resource));
    
    BufferHandle node(static_cast<uint32_t>(mNodes.size()));
    mNodes.emplace_back(node, resource);
    return node;
}

NO_DISCARD RenderGraphResource RenderGraphResourceRegistry::CloneResource(RenderGraphResource resource)
{
    GLEAM_ASSERT(resource != RenderGraphResource::nullHandle);
    return RenderGraphResource{resource, resource.GetVersion() + 1};
}

RenderGraphResourceNode& RenderGraphResourceRegistry::GetResourceNode(RenderGraphResource resource)
{
    GLEAM_ASSERT(resource != RenderGraphResource::nullHandle);
    return mNodes[resource];
}

const RenderGraphResourceNode& RenderGraphResourceRegistry::GetResourceNode(RenderGraphResource resource) const
{
    GLEAM_ASSERT(resource != RenderGraphResource::nullHandle);
    return mNodes[resource];
}

RenderGraphResourceEntry& RenderGraphResourceRegistry::GetResourceEntry(RenderGraphResource resource)
{
    const auto& node = GetResourceNode(resource);
    GLEAM_ASSERT(node.resource < mEntries.size());
    return mEntries[node.resource];
}
