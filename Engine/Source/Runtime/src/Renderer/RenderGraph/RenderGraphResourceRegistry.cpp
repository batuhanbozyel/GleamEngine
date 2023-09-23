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
	mBufferNodes.clear();
	mTextureNodes.clear();
}

NO_DISCARD TextureHandle RenderGraphResourceRegistry::CreateTexture(const RenderTextureDescriptor& descriptor, bool transient)
{
    auto uniqueId = static_cast<uint32_t>(mTextureNodes.size());
	auto& node = mTextureNodes.emplace_back(uniqueId, descriptor, transient);
    auto handle = TextureHandle(&node);
    return handle;
}

NO_DISCARD BufferHandle RenderGraphResourceRegistry::CreateBuffer(const BufferDescriptor& descriptor, bool transient)
{
    auto uniqueId = static_cast<uint32_t>(mBufferNodes.size());
	auto& node = mBufferNodes.emplace_back(uniqueId, descriptor, transient);
    auto handle = BufferHandle(&node);
    return handle;
}
