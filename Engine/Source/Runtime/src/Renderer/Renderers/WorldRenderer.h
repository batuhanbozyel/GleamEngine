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
};

class WorldRenderer : public IRenderer
{
public:
    
    virtual void OnCreate(const RenderContext& context) override;

	virtual void OnDestroy(const RenderContext& context) override;
    
    virtual void AddRenderPasses(RenderGraph& graph, RenderGraphBlackboard& blackboard) override;

	void RegisterShadingPipeline(const Material* material);

private:

	GraphicsDevice* mDevice = nullptr;
	HashMap<uint32_t, GraphicsPipelineHandle> mShadingPipelines;

};

} // namespace Gleam
