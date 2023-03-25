//
//  RenderGraphNode.h
//  GleamEngine
//
//  Created by Batuhan Bozyel on 11.03.2023.
//

#pragma once
#include "RenderGraphResource.h"

namespace Gleam {

struct RenderGraphContext;

struct RenderGraphNode
{
    const uint32_t uniqueId;
    uint32_t refCount = 0;
    
    RenderGraphNode(uint32_t uniqueId)
        : uniqueId(uniqueId)
    {
        
    }
};

template<typename PassData>
using RenderFunc = std::function<void(const RenderGraphContext& renderGraphContext, const PassData& passData)>;

struct RenderPassNode : public RenderGraphNode
{
    GLEAM_NONCOPYABLE(RenderPassNode);
    
    using PassCallback = std::function<void(const RenderGraphContext& renderGraphContext)>;
    
    TString name;
    
    std::any data;
    PassCallback callback;
    
    bool hasSideEffect = false;
    
    TArray<RenderGraphResource> renderTextureReads;
    TArray<RenderGraphResource> renderTextureWrites;
    TArray<RenderGraphResource> renderTextureCreates;
    
    TArray<RenderGraphResource> bufferReads;
    TArray<RenderGraphResource> bufferWrites;
    TArray<RenderGraphResource> bufferCreates;
    
    template<typename PassData>
    RenderPassNode(uint32_t uniqueId, const TStringView name, RenderFunc<PassData>&& execute)
        : RenderGraphNode(uniqueId), name(name), data(std::make_any<PassData>())
    {
        callback = [execute = move(execute), this](const RenderGraphContext& renderGraphContext)
        {
            std::invoke(execute, renderGraphContext, std::any_cast<const PassData&>(data));
        };
    }
    
    bool isCulled() const
    {
        return refCount == 0 && !hasSideEffect;
    }
};

struct RenderGraphResourceNode : public RenderGraphNode
{
    const RenderGraphResource resource;
    const uint32_t version;
    
    RenderPassNode* producer = nullptr;
    RenderPassNode* last = nullptr;
    
    RenderGraphResourceNode(uint32_t uniqueId, RenderGraphResource resource, uint32_t version)
        : RenderGraphNode(uniqueId), resource(resource), version(version)
    {
        
    }
};

} // namespace Gleam
