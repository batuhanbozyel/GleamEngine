//
//  RenderGraphBuilder.cpp
//  Runtime
//
//  Created by Batuhan Bozyel on 12.03.2023.
//

#include "gpch.h"
#include "RenderGraphBuilder.h"

using namespace Gleam;

static bool HasResource(const TArray<RenderGraphResource>& resources, RenderGraphResource resource)
{
    return std::find(resources.cbegin(), resources.cend(), resource) != resources.cend();
}

static RenderGraphResource Read(TArray<RenderGraphResource>& passReads, RenderGraphResource resource)
{
    return HasResource(passReads, resource) ? resource : passReads.emplace_back(resource);
}

static RenderGraphResource Write(TArray<RenderGraphResource>& passWrites, RenderGraphResource resource)
{
    return HasResource(passWrites, resource) ? resource : passWrites.emplace_back(resource);
}

RenderGraphBuilder::RenderGraphBuilder(RenderPassNode& node, RenderGraphResourceRegistry& registry)
    : mPassNode(node), mResourceRegistry(registry)
{
    
}

NO_DISCARD RenderTextureHandle RenderGraphBuilder::CreateRenderTexture(const TextureDescriptor& descriptor)
{
    RenderTextureHandle renderTexture = mResourceRegistry.CreateRT(descriptor);
    mPassNode.renderTextureCreates.emplace_back(renderTexture);
    return renderTexture;
}

NO_DISCARD RenderTextureHandle RenderGraphBuilder::WriteRenderTexture(RenderTextureHandle resource)
{
    if (mResourceRegistry.GetRenderTextureEntry(resource).transient == false)
        mPassNode.hasSideEffect = true;
    
    if (HasResource(mPassNode.renderTextureCreates, resource))
    {
        return Write(mPassNode.renderTextureWrites, resource);
    }
    else
    {
        ReadRenderTexture(resource);
        return Write(mPassNode.renderTextureWrites, mResourceRegistry.CloneRT(resource));
    }
}

RenderTextureHandle RenderGraphBuilder::ReadRenderTexture(RenderTextureHandle resource)
{
    return Read(mPassNode.renderTextureReads, resource);
}

NO_DISCARD BufferHandle RenderGraphBuilder::CreateBuffer(const BufferDescriptor& descriptor)
{
    BufferHandle buffer = mResourceRegistry.CreateBuffer(descriptor);
    mPassNode.bufferCreates.emplace_back(buffer);
    return buffer;
}

NO_DISCARD BufferHandle RenderGraphBuilder::WriteBuffer(BufferHandle resource)
{
    if (mResourceRegistry.GetBufferEntry(resource).transient == false)
        mPassNode.hasSideEffect = true;
    
    if (HasResource(mPassNode.bufferCreates, resource))
    {
        return Write(mPassNode.bufferWrites, resource);
    }
    else
    {
        ReadBuffer(resource);
        auto clone = mResourceRegistry.CloneBuffer(resource);
        mPassNode.bufferWrites.emplace_back(clone);
        return clone;
    }
}

BufferHandle RenderGraphBuilder::ReadBuffer(BufferHandle resource)
{
    return Read(mPassNode.bufferReads, resource);
}
