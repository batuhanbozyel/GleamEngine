//
//  RenderGraphBuilder.cpp
//  Runtime
//
//  Created by Batuhan Bozyel on 12.03.2023.
//

#include "gpch.h"
#include "RenderGraphBuilder.h"

using namespace Gleam;

RenderGraphBuilder::RenderGraphBuilder(RenderPassNode& node, RenderGraphResourceRegistry& registry)
    : mPassNode(node), mResourceRegistry(registry)
{
    
}

NO_DISCARD TextureHandle RenderGraphBuilder::UseColorBuffer(const TextureHandle& attachment)
{
    return mPassNode.colorAttachments.emplace_back(WriteTexture(attachment));
}

NO_DISCARD TextureHandle RenderGraphBuilder::UseDepthBuffer(const TextureHandle& attachment)
{
    return mPassNode.depthAttachment = WriteTexture(attachment);
}

NO_DISCARD BufferHandle RenderGraphBuilder::CreateBuffer(const BufferDescriptor& descriptor)
{
	auto handle = mResourceRegistry.CreateBuffer(descriptor);
    return mPassNode.bufferCreates.emplace_back(handle);
}

NO_DISCARD TextureHandle RenderGraphBuilder::CreateTexture(const RenderTextureDescriptor& descriptor)
{
    auto handle = mResourceRegistry.CreateTexture(descriptor);
    return mPassNode.textureCreates.emplace_back(handle);
}

NO_DISCARD BufferHandle RenderGraphBuilder::WriteBuffer(const BufferHandle& resource)
{
    GLEAM_ASSERT(resource.IsValid());
    
	if (HasResource(mPassNode.bufferWrites, resource)) { return resource; }

	if (resource.node->transient == false) { mPassNode.hasSideEffect = true; }
	resource.node->producers.push_back(&mPassNode);

	auto clone = BufferHandle(resource.node, resource.version + 1, ResourceAccess::Write);
	mPassNode.bufferWrites.emplace_back(clone);
	return clone;
}

NO_DISCARD TextureHandle RenderGraphBuilder::WriteTexture(const TextureHandle& resource)
{
    GLEAM_ASSERT(resource.IsValid());
    
	if (HasResource(mPassNode.textureWrites, resource)) { return resource; }
    
	if (resource.node->transient == false) { mPassNode.hasSideEffect = true; }
	resource.node->producers.push_back(&mPassNode);

	auto clone = TextureHandle(resource.node, resource.version + 1, ResourceAccess::Write);
	mPassNode.textureWrites.emplace_back(clone);
	return clone;
}

NO_DISCARD BufferHandle RenderGraphBuilder::ReadBuffer(const BufferHandle& resource)
{
    GLEAM_ASSERT(!HasResource(mPassNode.bufferCreates, resource) && !HasResource(mPassNode.bufferWrites, resource));
    GLEAM_ASSERT(resource.IsValid());
    
    if (HasResource(mPassNode.bufferReads, resource)) { return resource; }
    
    auto clone = BufferHandle(resource.node, resource.version, ResourceAccess::Read);
    mPassNode.bufferReads.emplace_back(clone);
    return clone;
}

NO_DISCARD TextureHandle RenderGraphBuilder::ReadTexture(const TextureHandle& resource)
{
    GLEAM_ASSERT(!HasResource(mPassNode.textureCreates, resource) && !HasResource(mPassNode.textureWrites, resource));
    GLEAM_ASSERT(resource.IsValid());
    
    if (HasResource(mPassNode.textureReads, resource)) { return resource; }
    
    auto clone = TextureHandle(resource.node, resource.version, ResourceAccess::Read);
    mPassNode.textureReads.emplace_back(clone);
    return clone;
}
