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

NO_DISCARD TextureHandle RenderGraphResourceRegistry::CreateTexture(const RenderTextureDescriptor& descriptor)
{
    auto uniqueId = static_cast<uint32_t>(mTextureNodes.size());
	auto node = RenderGraphTextureNode(uniqueId, descriptor, true);
    return TextureHandle(&mTextureNodes.emplace_back(uniqueId, descriptor, true));
}

NO_DISCARD BufferHandle RenderGraphResourceRegistry::CreateBuffer(const BufferDescriptor& descriptor)
{
    auto uniqueId = static_cast<uint32_t>(mBufferNodes.size());
	auto node = RenderGraphBufferNode(uniqueId, descriptor, true);
    return BufferHandle(&mBufferNodes.emplace_back(uniqueId, descriptor, true));
}
