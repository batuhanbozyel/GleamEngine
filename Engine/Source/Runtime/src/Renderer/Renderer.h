//
//  Renderer.h
//  Runtime
//
//  Created by Batuhan Bozyel on 31.10.2022.
//

#pragma once
#include "Buffer.h"
#include "Shader.h"
#include "Pipeline.h"
#include "RendererConfig.h"
#include "RenderPassDescriptor.h"
#include "Shaders/ShaderTypes.h"
#include "RenderGraph/RenderGraph.h"
#include "RenderGraph/RenderGraphBlackboard.h"
#include "World/Entity.h"

namespace Gleam {

class World;
class RenderSystem;
class RenderSurface;
class RenderPipeline;
class GraphicsDevice;
class RenderSceneProxy;
class ResourceReleaseQueue;

struct RenderContext
{
	ResourceReleaseQueue* releaseQueue = nullptr;
	GPUAllocator* allocator = nullptr;
	GraphicsDevice* device = nullptr;
	RenderSurface* surface = nullptr;
};

struct CameraRenderData
{
	CameraUniforms uniforms = {};
	EntityHandle entity = InvalidEntity;
};

struct SkyAtmosphereRenderData
{
	SkyAtmosphereUniforms uniforms = {};
	SkyAtmosphereParameters params = {};
	TextureHandle transmittanceLut = TextureHandle();
	TextureHandle multiScatterLut = TextureHandle();
	EntityHandle entity = InvalidEntity;
};

struct SceneRenderingData
{
    const RenderSceneProxy* sceneProxy = nullptr;
    const World* world = nullptr;
	CameraRenderData camera = {};
	SkyAtmosphereRenderData atmosphere = {};
    TextureHandle backbuffer = TextureHandle();
	TextureHandle sceneTarget = TextureHandle();
};

class IRenderer
{
public:
    
    friend class RenderSystem;
	friend class RenderPipeline;
    friend class GraphicsDevice;
    
    virtual ~IRenderer() = default;

protected:

	virtual void OnCreate(RenderContext& context) {}

	virtual void OnDestroy(RenderContext& context) {}

	virtual void AddRenderPasses(RenderGraph& graph, RenderGraphBlackboard& blackboard) = 0;

};

} // namespace Gleam
