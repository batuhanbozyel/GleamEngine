//
//  Renderer.h
//  Runtime
//
//  Created by Batuhan Bozyel on 31.10.2022.
//

#pragma once
#include "ShaderTypes.h"
#include "RenderGraph/RenderGraph.h"

namespace Gleam {

class View;
class RendererContext;

struct RenderingData
{
    RenderTextureHandle colorTarget = { RenderGraphResource::invalidHandle };
    RenderTextureHandle depthTarget = { RenderGraphResource::invalidHandle };
    BufferHandle cameraBuffer;
};

class IRenderer
{
public:
    
    friend class View;
    friend class RendererContext;
    
    virtual ~IRenderer() = default;

protected:

	virtual void OnCreate(RendererContext* context) {}

	virtual void AddRenderPasses(RenderGraph& graph, const RenderingData& renderData) = 0;

};

} // namespace Gleam
