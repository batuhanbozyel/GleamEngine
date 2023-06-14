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

const RefCounted<Buffer>& RenderGraphResourceRegistry::GetBuffer(BufferHandle handle) const
{
    return static_cast<RenderGraphBufferEntry*>(GetResourceEntry(handle))->buffer;
}

const RefCounted<RenderTexture>& RenderGraphResourceRegistry::GetRenderTexture(RenderTextureHandle handle) const
{
    return static_cast<RenderGraphRenderTextureEntry*>(GetResourceEntry(handle))->renderTexture;
}

NO_DISCARD RenderTextureHandle RenderGraphResourceRegistry::CreateRT(const TextureDescriptor& descriptor)
{
    RenderTextureHandle resource(static_cast<uint32_t>(mEntries.size()));
    mEntries.emplace_back(CreateScope<RenderGraphRenderTextureEntry>(descriptor, resource));
    
    RenderTextureHandle node(static_cast<uint32_t>(mNodes.size()));
    mNodes.emplace_back(node, resource);
    return node;
}

NO_DISCARD BufferHandle RenderGraphResourceRegistry::CreateBuffer(const BufferDescriptor& descriptor)
{
    BufferHandle resource(static_cast<uint32_t>(mEntries.size()));
    mEntries.emplace_back(CreateScope<RenderGraphBufferEntry>(descriptor, resource));
    
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

RenderGraphResourceEntry* RenderGraphResourceRegistry::GetResourceEntry(RenderGraphResource resource) const
{
    const auto& node = GetResourceNode(resource);
    GLEAM_ASSERT(node.resource < mEntries.size());
    return mEntries[node.resource].get();
}
