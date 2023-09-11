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
}

NO_DISCARD TextureHandle RenderGraphResourceRegistry::CreateRT(const RenderTextureDescriptor& descriptor)
{
    auto uniqueId = static_cast<uint32_t>(mNodes.size());
    TextureHandle handle(uniqueId);
    mNodes.emplace_back(handle, true);
    return handle;
}

NO_DISCARD BufferHandle RenderGraphResourceRegistry::CreateBuffer(const BufferDescriptor& descriptor)
{
    auto uniqueId = static_cast<uint32_t>(mNodes.size());
    auto& node = mNodes.emplace_back(uniqueId, true);
    return BufferHandle handle(node);
}

RenderGraphResourceNode& RenderGraphResourceRegistry::GetResourceNode(ResourceHandle resource)
{
    GLEAM_ASSERT(resource != ResourceHandle::nullHandle);
    return mNodes[resource];
}

const RenderGraphResourceNode& RenderGraphResourceRegistry::GetResourceNode(ResourceHandle resource) const
{
    GLEAM_ASSERT(resource != ResourceHandle::nullHandle);
    return mNodes[resource];
}
