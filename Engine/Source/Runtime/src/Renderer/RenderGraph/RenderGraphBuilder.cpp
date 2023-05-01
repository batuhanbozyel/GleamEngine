//
//  RenderGraphBuilder.cpp
//  Runtime
//
//  Created by Batuhan Bozyel on 12.03.2023.
//

#include "gpch.h"
#include "RenderGraphBuilder.h"

using namespace Gleam;

static RenderGraphResource EmplaceIfNeeded(TArray<RenderGraphResource>& passResources, RenderGraphResource resource)
{
    return HasResource(passResources, resource) ? resource : passResources.emplace_back(resource);
}

RenderGraphBuilder::RenderGraphBuilder(RenderPassNode& node, RenderGraphResourceRegistry& registry)
    : mPassNode(node), mResourceRegistry(registry)
{
    
}

NO_DISCARD RenderTextureHandle RenderGraphBuilder::UseColorBuffer(RenderPassAttachment attachment)
{;
    attachment.texture = WriteRenderTexture(attachment.texture);
    mPassNode.colorAttachments.push_back(attachment);
    return attachment.texture;
}

NO_DISCARD RenderTextureHandle RenderGraphBuilder::UseDepthBuffer(RenderPassAttachment attachment)
{
    mPassNode.depthAttachment.texture = WriteRenderTexture(attachment.texture);
    return mPassNode.depthAttachment.texture;
}

NO_DISCARD BufferHandle RenderGraphBuilder::CreateBuffer(const BufferDescriptor& descriptor)
{
    return mPassNode.bufferCreates.emplace_back(mResourceRegistry.CreateBuffer(descriptor));
}

NO_DISCARD RenderTextureHandle RenderGraphBuilder::CreateRenderTexture(const TextureDescriptor& descriptor)
{
    return mPassNode.renderTextureCreates.emplace_back(mResourceRegistry.CreateRT(descriptor));
}

NO_DISCARD BufferHandle RenderGraphBuilder::WriteBuffer(BufferHandle resource)
{
    if (mResourceRegistry.GetBufferEntry(resource).transient == false) { mPassNode.hasSideEffect = true; }
    
    if (!HasResource(mPassNode.bufferCreates, resource))
    {
        resource = mResourceRegistry.CloneBuffer(resource);
    }
    return EmplaceIfNeeded(mPassNode.bufferWrites, resource);
}

NO_DISCARD RenderTextureHandle RenderGraphBuilder::WriteRenderTexture(RenderTextureHandle resource)
{
    if (mResourceRegistry.GetRenderTextureEntry(resource).transient == false) { mPassNode.hasSideEffect = true; }
    
    if (!HasResource(mPassNode.renderTextureCreates, resource))
    {
        resource = mResourceRegistry.CloneRT(resource);
    }
    return EmplaceIfNeeded(mPassNode.renderTextureWrites, resource);
}

BufferHandle RenderGraphBuilder::ReadBuffer(BufferHandle resource)
{
    GLEAM_ASSERT(!HasResource(mPassNode.bufferCreates, resource) && !HasResource(mPassNode.bufferWrites, resource));
    return EmplaceIfNeeded(mPassNode.bufferReads, resource);
}

RenderTextureHandle RenderGraphBuilder::ReadRenderTexture(RenderTextureHandle resource)
{
    GLEAM_ASSERT(!HasResource(mPassNode.renderTextureCreates, resource) && !HasResource(mPassNode.renderTextureWrites, resource));
    return EmplaceIfNeeded(mPassNode.renderTextureReads, resource);
}
