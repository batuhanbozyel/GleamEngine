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
    return mPassNode.resourceCreates.emplace_back(mResourceRegistry.CreateBuffer(descriptor));
}

NO_DISCARD RenderTextureHandle RenderGraphBuilder::CreateRenderTexture(const TextureDescriptor& descriptor)
{
    return mPassNode.resourceCreates.emplace_back(mResourceRegistry.CreateRT(descriptor));
}

NO_DISCARD RenderGraphResource RenderGraphBuilder::WriteResource(RenderGraphResource resource)
{
    if (mResourceRegistry.GetResourceEntry(resource)->transient == false) { mPassNode.hasSideEffect = true; }
    
    mResourceRegistry.GetResourceNode(resource).producer = &mPassNode;
    return EmplaceIfNeeded(mPassNode.resourceWrites, resource);
}

NO_DISCARD BufferHandle RenderGraphBuilder::WriteBuffer(BufferHandle resource)
{
    return BufferHandle(WriteResource(resource));
}

NO_DISCARD RenderTextureHandle RenderGraphBuilder::WriteRenderTexture(RenderTextureHandle resource)
{
    return RenderTextureHandle(WriteResource(resource));
}

BufferHandle RenderGraphBuilder::ReadBuffer(BufferHandle resource)
{
    GLEAM_ASSERT(!HasResource(mPassNode.resourceCreates, resource) && !HasResource(mPassNode.resourceWrites, resource));
    return EmplaceIfNeeded(mPassNode.resourceReads, resource);
}

RenderTextureHandle RenderGraphBuilder::ReadRenderTexture(RenderTextureHandle resource)
{
    GLEAM_ASSERT(!HasResource(mPassNode.resourceCreates, resource) && !HasResource(mPassNode.resourceWrites, resource));
    return EmplaceIfNeeded(mPassNode.resourceReads, resource);
}
