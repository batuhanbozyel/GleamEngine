//
//  RenderGraphNode.h
//  GleamEngine
//
//  Created by Batuhan Bozyel on 11.03.2023.
//

#pragma once
#include "RenderGraphResource.h"

#include <functional>

namespace Gleam {

struct RenderGraphContext;

template<typename PassData>
using RenderFunc = std::function<void(const RenderGraphContext& renderGraphContext, const PassData& passData)>;

struct RenderGraphNode
{
    const uint32_t uniqueId;
    uint32_t refCount = 0;
    
    RenderGraphNode(uint32_t uniqueId)
        : uniqueId(uniqueId)
    {
        
    }
};

struct RenderPassNode final : public RenderGraphNode
{
    GLEAM_NONCOPYABLE(RenderPassNode);
    
    using PassCallback = std::function<void(const RenderGraphContext& renderGraphContext)>;
    
    TString name;
    
    std::any data;
    PassCallback callback;
    
    bool hasSideEffect = false;
    
    TArray<RenderGraphResource> resourceReads;
    TArray<RenderGraphResource> resourceWrites;
    TArray<RenderGraphResource> resourceCreates;
    
    TArray<RenderGraphResource> colorAttachments;
    RenderGraphResource depthAttachment = RenderGraphResource::nullHandle;
    
    HashSet<RenderPassNode*> dependents;
    
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
    
    bool isCustomPass() const
    {
        return colorAttachments.empty() && depthAttachment == RenderGraphResource::nullHandle;
    }
};

struct RenderGraphResourceNode : public RenderGraphNode
{
    const RenderGraphResource resource;
    TArray<RenderPassNode*> producers;
    
    RenderGraphResourceNode(uint32_t uniqueId, RenderGraphResource resource)
        : RenderGraphNode(uniqueId), resource(resource)
    {
        
    }
};

} // namespace Gleam
