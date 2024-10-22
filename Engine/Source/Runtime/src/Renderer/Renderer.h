//
//  Renderer.h
//  Runtime
//
//  Created by Batuhan Bozyel on 31.10.2022.
//

#pragma once
#include "Shader.h"
#include "RendererConfig.h"
#include "RenderPassDescriptor.h"
#include "PipelineStateDescriptor.h"
#include "Shaders/ShaderTypes.h"
#include "RenderGraph/RenderGraph.h"
#include "RenderGraph/RenderGraphBlackboard.h"

namespace Gleam {

class World;
class RenderSystem;
class GraphicsDevice;
class RenderSceneProxy;

struct SceneRenderingData
{
    const RenderSceneProxy* sceneProxy = nullptr;
    const World* world = nullptr;
    TextureHandle backbuffer;
    BufferHandle cameraBuffer;
};

class IRenderer
{
public:
    
    friend class RenderSystem;
    friend class GraphicsDevice;
    
    virtual ~IRenderer() = default;

protected:

	virtual void OnCreate(GraphicsDevice* device) {}

	virtual void OnDestroy(GraphicsDevice* device) {}

	virtual void AddRenderPasses(RenderGraph& graph, RenderGraphBlackboard& blackboard) = 0;

};

} // namespace Gleam
