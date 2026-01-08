//
//  WorldRenderer.h
//  Runtime
//
//  Created by Batuhan Bozyel on 20.10.2022.
//

#pragma once
#include "Renderer/Renderer.h"
#include "Renderer/Mesh.h"
#include "Renderer/Material/Material.h"
#include "Renderer/Material/MaterialInstance.h"

namespace Gleam {

struct MaterialDescriptor;

struct WorldRenderingData
{
    TextureHandle colorTarget = TextureHandle();
    TextureHandle depthTarget = TextureHandle();
	TextureHandle transmittanceLut = TextureHandle();
	TextureHandle multiScatterLut = TextureHandle();
	TextureHandle brdfLut = TextureHandle();
	TextureHandle specularReflection = TextureHandle();
	TextureHandle diffuseReflection = TextureHandle();
};

class WorldRenderer : public IRenderer
{
public:
    
    virtual void OnCreate(RenderContext& context) override;

	virtual void OnDestroy(RenderContext& context) override;
    
    virtual void AddRenderPasses(RenderGraph& graph, RenderGraphBlackboard& blackboard) override;

	void RegisterShadingPipeline(const MaterialDescriptor& material, uint32_t hash);

	const Texture& GetBRDFLutTexture() const
	{
		return mBRDFLutTexture;
	}

private:

	Texture mBRDFLutTexture;

	ComputePipelineHandle mBRDFLutPipeline;

	GraphicsDevice* mDevice = nullptr;

	HashMap<uint32_t, GraphicsPipelineHandle> mShadingPipelines;

};

} // namespace Gleam
