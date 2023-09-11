//
//  RenderGraphBuilder.cpp
//  Runtime
//
//  Created by Batuhan Bozyel on 12.03.2023.
//

#include "gpch.h"
#include "RenderGraphBuilder.h"

using namespace Gleam;

static ResourceHandle EmplaceIfNeeded(TArray<ResourceHandle>& passResources, ResourceHandle resource)
{
    return HasResource(passResources, resource) ? resource : passResources.emplace_back(resource);
}

RenderGraphBuilder::RenderGraphBuilder(RenderPassNode& node, RenderGraphResourceRegistry& registry)
    : mPassNode(node), mResourceRegistry(registry)
{
    
}

NO_DISCARD TextureHandle RenderGraphBuilder::UseColorBuffer(TextureHandle attachment)
{
    return mPassNode.colorAttachments.emplace_back(WriteRenderTexture(attachment));
}

NO_DISCARD TextureHandle RenderGraphBuilder::UseDepthBuffer(TextureHandle attachment)
{
    return mPassNode.depthAttachment = WriteRenderTexture(attachment);
}

NO_DISCARD BufferHandle RenderGraphBuilder::CreateBuffer(const BufferDescriptor& descriptor)
{
	auto handle = mResourceRegistry.CreateBuffer(descriptor);
    mPassNode.resourceCreates.emplace_back(handle);
	return handle;
}

NO_DISCARD TextureHandle RenderGraphBuilder::CreateRenderTexture(const RenderTextureDescriptor& descriptor)
{
    auto handle = mResourceRegistry.CreateRT(descriptor);
    mPassNode.resourceCreates.emplace_back(handle);
    return handle;
}

NO_DISCARD BufferHandle RenderGraphBuilder::WriteBuffer(const BufferHandle& resource)
{
    return WriteResource(resource);
}

NO_DISCARD TextureHandle RenderGraphBuilder::WriteRenderTexture(TextureHandle resource)
{
    return WriteResource(resource);
}

BufferHandle RenderGraphBuilder::ReadBuffer(const BufferHandle& resource)
{
    GLEAM_ASSERT(!HasResource(mPassNode.resourceCreates, resource) && !HasResource(mPassNode.resourceWrites, resource));
    return EmplaceIfNeeded(mPassNode.resourceReads, resource);
}

TextureHandle RenderGraphBuilder::ReadRenderTexture(TextureHandle resource)
{
    GLEAM_ASSERT(!HasResource(mPassNode.resourceCreates, resource) && !HasResource(mPassNode.resourceWrites, resource));
    return EmplaceIfNeeded(mPassNode.resourceReads, resource);
}
