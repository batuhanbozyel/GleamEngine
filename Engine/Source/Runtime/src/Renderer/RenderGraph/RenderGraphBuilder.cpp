//
//  RenderGraphBuilder.cpp
//  Runtime
//
//  Created by Batuhan Bozyel on 12.03.2023.
//

#include "gpch.h"
#include "RenderGraphBuilder.h"

using namespace Gleam;

static bool HasResource(const TArray<RenderGraphResource>& resources, RenderGraphResource uniqueId)
{
    return std::find(resources.cbegin(), resources.cend(), uniqueId) != resources.cend();
}

static RenderGraphResource Read(TArray<RenderGraphResource>& passReads, RenderGraphResource uniqueId)
{
    return HasResource(passReads, uniqueId) ? uniqueId : passReads.emplace_back(uniqueId);
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
        if (!HasResource(mPassNode.renderTextureWrites, resource))
            mPassNode.renderTextureWrites.emplace_back(resource);
        
        return resource;
    }
    else
    {
        Read(resource);
        auto clone = mResourceRegistry.CloneRT(resource);
        mPassNode.renderTextureWrites.emplace_back(clone);
        return clone;
    }
}

RenderTextureHandle RenderGraphBuilder::Read(RenderTextureHandle resource)
{
    ::Read(mPassNode.renderTextureReads, resource);
    return resource;
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
        if (!HasResource(mPassNode.bufferWrites, resource))
            mPassNode.bufferWrites.emplace_back(resource);
        
        return resource;
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
    ::Read(mPassNode.bufferReads, resource);
    return resource;
}
