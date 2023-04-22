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
    mRenderTextureNodes.clear();
    mRenderTextureEntries.clear();
    
    mBufferNodes.clear();
    mBufferEntries.clear();
}

const RefCounted<Buffer>& RenderGraphResourceRegistry::GetBuffer(BufferHandle buffer) const
{
    GLEAM_ASSERT(buffer != RenderGraphResource::nullHandle);
    
    const auto& node = mBufferNodes[buffer];
    return mBufferEntries[node.resource].buffer;
}

const RefCounted<RenderTexture>& RenderGraphResourceRegistry::GetRenderTexture(RenderTextureHandle renderTexture) const
{
    GLEAM_ASSERT(renderTexture != RenderGraphResource::nullHandle);
    
    const auto& node = mRenderTextureNodes[renderTexture];
    return mRenderTextureEntries[node.resource].renderTexture;
}

NO_DISCARD RenderTextureHandle RenderGraphResourceRegistry::CreateRT(const TextureDescriptor& descriptor)
{
    RenderTextureHandle resource(static_cast<uint32_t>(mRenderTextureEntries.size()));
    mRenderTextureEntries.emplace_back(descriptor, resource);
    
    RenderTextureHandle node(static_cast<uint32_t>(mRenderTextureNodes.size()));
    mRenderTextureNodes.emplace_back(node, resource);
    return node;
}

NO_DISCARD RenderTextureHandle RenderGraphResourceRegistry::CloneRT(RenderTextureHandle resource)
{
    GLEAM_ASSERT(resource != RenderGraphResource::nullHandle);
    
    const auto& node = mRenderTextureNodes[resource];
    auto& entry = mRenderTextureEntries[node.resource];
    
    RenderTextureHandle clone(static_cast<uint32_t>(mRenderTextureNodes.size()));
    mRenderTextureNodes.emplace_back(clone, node.resource);
    return clone;
}

NO_DISCARD BufferHandle RenderGraphResourceRegistry::CreateBuffer(const BufferDescriptor& descriptor)
{
    BufferHandle resource(static_cast<uint32_t>(mBufferEntries.size()));
    mBufferEntries.emplace_back(descriptor, resource);
    
    BufferHandle node(static_cast<uint32_t>(mBufferNodes.size()));
    mBufferNodes.emplace_back(node, resource);
    return node;
}

NO_DISCARD BufferHandle RenderGraphResourceRegistry::CloneBuffer(BufferHandle resource)
{
    GLEAM_ASSERT(resource != RenderGraphResource::nullHandle);
    
    const auto& node = mBufferNodes[resource];
    auto& entry = mBufferEntries[node.resource];
    
    BufferHandle clone(static_cast<uint32_t>(mBufferNodes.size()));
    mBufferNodes.emplace_back(clone, node.resource);
    return clone;
}

RenderGraphBufferEntry& RenderGraphResourceRegistry::GetBufferEntry(RenderGraphResource resource)
{
    const auto& node = mBufferNodes[resource];
    GLEAM_ASSERT(node.resource < mBufferEntries.size());
    return mBufferEntries[node.resource];
}

RenderGraphRenderTextureEntry& RenderGraphResourceRegistry::GetRenderTextureEntry(RenderGraphResource resource)
{
    const auto& node = mRenderTextureNodes[resource];
    GLEAM_ASSERT(node.resource < mRenderTextureEntries.size());
    return mRenderTextureEntries[node.resource];
}
