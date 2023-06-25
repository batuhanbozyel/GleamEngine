//
//  Renderer.h
//  Runtime
//
//  Created by Batuhan Bozyel on 31.10.2022.
//

#pragma once
#include "ShaderTypes.h"
#include "RendererConfig.h"
#include "RenderPassDescriptor.h"
#include "PipelineStateDescriptor.h"
#include "RenderGraph/RenderGraph.h"
#include "RenderGraph/RenderGraphBlackboard.h"

namespace Gleam {

class RenderSystem;
class RendererContext;

struct RenderingData
{
    RenderTextureHandle backbuffer;
    RendererConfig config;
};

class IRenderer
{
public:
    
    friend class RenderSystem;
    friend class RendererContext;
    
    virtual ~IRenderer() = default;

protected:

	virtual void OnCreate(RendererContext& context) {}

	virtual void OnDestroy() {}

	virtual void AddRenderPasses(RenderGraph& graph, RenderGraphBlackboard& blackboard) = 0;

};

} // namespace Gleam
