#pragma once
#include "Renderer/Renderer.h"

namespace Gleam {

class Material;

struct BRDFData
{
	TextureHandle brdfLut = TextureHandle();
	TextureHandle ggxEssLut = TextureHandle();
	TextureHandle ggxEAvgLut = TextureHandle();
};

class BRDFRenderer : public IRenderer
{
public:
    
    virtual void OnCreate(const RenderContext& context) override;

	virtual void OnDestroy(const RenderContext& context) override;
    
    virtual void AddRenderPasses(RenderGraph& graph, RenderGraphBlackboard& blackboard) override;

	virtual RenderStage GetStage() const override { return RenderStage::BeforeRendering; }

private:

	Texture mBRDFLutTexture;
	Texture mGGXEssLutTexture;
	Texture mGGXEAvgLutTexture;

	ComputePipelineHandle mBRDFLutPipeline;
	ComputePipelineHandle mGGXEssLutPipeline;
	ComputePipelineHandle mGGXEAvgLutPipeline;

};

} // namespace Gleam
