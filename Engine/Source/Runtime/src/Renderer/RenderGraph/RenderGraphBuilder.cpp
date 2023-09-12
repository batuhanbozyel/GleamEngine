//
//  RenderGraphBuilder.cpp
//  Runtime
//
//  Created by Batuhan Bozyel on 12.03.2023.
//

#include "gpch.h"
#include "RenderGraphBuilder.h"

using namespace Gleam;

template <typename T>
concept ResourceType = std::is_base_of<ResourceHandle, T>::value;

template <ResourceType T>
static bool HasResource(const TArray<T>& resources, T resource)
{
	return std::find(resources.cbegin(), resources.cend(), resource) != resources.cend();
}

template <ResourceType T>
static T EmplaceIfNeeded(TArray<T>& passResources, T resource)
{
    return HasResource(passResources, resource) ? resource : passResources.emplace_back(resource);
}

RenderGraphBuilder::RenderGraphBuilder(RenderPassNode& node, RenderGraphResourceRegistry& registry)
    : mPassNode(node), mResourceRegistry(registry)
{
    
}

NO_DISCARD TextureHandle RenderGraphBuilder::UseColorBuffer(TextureHandle attachment)
{
    return mPassNode.colorAttachments.emplace_back(WriteTexture(attachment));
}

NO_DISCARD TextureHandle RenderGraphBuilder::UseDepthBuffer(TextureHandle attachment)
{
    return mPassNode.depthAttachment = WriteTexture(attachment);
}

NO_DISCARD BufferHandle RenderGraphBuilder::CreateBuffer(const BufferDescriptor& descriptor)
{
	auto handle = mResourceRegistry.CreateBuffer(descriptor);
    mPassNode.bufferCreates.emplace_back(handle);
	return handle;
}

NO_DISCARD TextureHandle RenderGraphBuilder::CreateTexture(const RenderTextureDescriptor& descriptor)
{
    auto handle = mResourceRegistry.CreateTexture(descriptor);
    mPassNode.textureCreates.emplace_back(handle);
    return handle;
}

NO_DISCARD BufferHandle RenderGraphBuilder::WriteBuffer(const BufferHandle& resource)
{
	if (HasResource(mPassNode.bufferWrites, resource)) { return resource; }

	if (resource.node->transient == false) { mPassNode.hasSideEffect = true; }
	resource.node->producers.push_back(&mPassNode);

	auto clone = BufferHandle(resource.node, resource.version + 1, ResourceAccess::Write);
	mPassNode.bufferWrites.emplace_back(clone);
	return clone;
}

NO_DISCARD TextureHandle RenderGraphBuilder::WriteTexture(TextureHandle resource)
{
	if (HasResource(mPassNode.textureWrites, resource)) { return resource; }

	if (resource.node->transient == false) { mPassNode.hasSideEffect = true; }
	resource.node->producers.push_back(&mPassNode);

	auto clone = TextureHandle(resource.node, resource.version + 1, ResourceAccess::Write);
	mPassNode.textureWrites.emplace_back(clone);
	return clone;
}

BufferHandle RenderGraphBuilder::ReadBuffer(const BufferHandle& resource)
{
    GLEAM_ASSERT(!HasResource(mPassNode.bufferCreates, resource) && !HasResource(mPassNode.bufferWrites, resource));
    return EmplaceIfNeeded(mPassNode.bufferReads, resource);
}

TextureHandle RenderGraphBuilder::ReadTexture(TextureHandle resource)
{
    GLEAM_ASSERT(!HasResource(mPassNode.textureCreates, resource) && !HasResource(mPassNode.textureWrites, resource));
    return EmplaceIfNeeded(mPassNode.textureReads, resource);
}
