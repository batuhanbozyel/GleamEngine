//
//  WorldRenderer.h
//  Runtime
//
//  Created by Batuhan Bozyel on 20.10.2022.
//

#pragma once
#include "Renderer/Renderer.h"

namespace Gleam {

class Material;

struct WorldRenderingData
{
    TextureHandle colorTarget = TextureHandle();
    TextureHandle depthTarget = TextureHandle();
	TextureHandle transmittanceLut = TextureHandle();
	TextureHandle multiScatterLut = TextureHandle();
	TextureHandle brdfLut = TextureHandle();
	TextureHandle ggxEssLut = TextureHandle();
	TextureHandle ggxEAvgLut = TextureHandle();
	TextureHandle specularReflection = TextureHandle();
	TextureHandle diffuseReflection = TextureHandle();
	TextureHandle shadowTexture = TextureHandle();
	TextureHandle aoTexture = TextureHandle();
	TextureHandle reflectionTexture = TextureHandle();
};

class WorldRenderer : public IRenderer
{
public:
    
    virtual void OnCreate(const RenderContext& context) override;

	virtual void OnDestroy(const RenderContext& context) override;
    
    virtual void AddRenderPasses(RenderGraph& graph, RenderGraphBlackboard& blackboard) override;

	virtual void RegisterShadingPipeline(const Material* material) override;

private:

	void AddVisibilityPass(RenderGraph& graph, RenderGraphBlackboard& blackboard);

	void AddForwardPass(RenderGraph& graph, RenderGraphBlackboard& blackboard);

	void MakeWorldRenderingData(const RenderGraph& graph, RenderGraphBlackboard& blackboard, RenderGraphBuilder& builder, WorldRenderingData& passData);

	GraphicsDevice* mDevice = nullptr;
	HashMap<uint32_t, MeshPipelineHandle> mMeshShadingPipelines;
	HashMap<uint32_t, ComputePipelineHandle> mVisibilityShadingPipelines;

};

} // namespace Gleam
