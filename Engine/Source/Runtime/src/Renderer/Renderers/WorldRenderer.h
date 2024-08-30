//
//  WorldRenderer.h
//  Runtime
//
//  Created by Batuhan Bozyel on 20.10.2022.
//

#pragma once
#include "Renderer/Renderer.h"

namespace Gleam {

struct WorldRenderingData
{
    TextureHandle colorTarget;
    TextureHandle depthTarget;
    BufferHandle cameraBuffer;
};

class WorldRenderer : public IRenderer
{
public:
    
    virtual void OnCreate(GraphicsDevice* device) override;
    
    virtual void AddRenderPasses(RenderGraph& graph, RenderGraphBlackboard& blackboard) override;

private:
    
    Shader mForwardPassVertexShader;
    Shader mForwardPassFragmentShader;
	HashMap<uint32_t, PipelineStateDescriptor> mShadingPipelines;

};

} // namespace Gleam
