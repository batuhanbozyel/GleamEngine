//
//  RenderGraphResourceEntry.h
//  GleamEngine
//
//  Created by Batuhan Bozyel on 24.03.2023.
//
#pragma once
#include "RenderGraphNode.h"

#include "Renderer/Buffer.h"
#include "Renderer/Texture.h"

namespace Gleam {

struct RenderGraphResourceEntry
{
    const RenderGraphResource resource;
    const bool transient;
    
    RenderPassNode* creator = nullptr;
    RenderPassNode* lastModifier = nullptr;
    RenderPassNode* lastReference = nullptr;
    
    uint32_t refCount = 0;
    
    RenderGraphResourceEntry(RenderGraphResource resource, bool transient)
        : resource(resource), transient(transient)
    {
        
    }
};

} // namespace Gleam
