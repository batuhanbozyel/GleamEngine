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

GCLASS(BRDFRenderer, "92D2C170-BBC3-4D0A-B70C-223CBDB20092", Renderer(RenderStage::BeforeRendering, true), PrettyName("BRDF")) : public IRenderer
{
public:
    
    virtual void OnCreate(const RenderContext& context) override;

	virtual void OnDestroy(const RenderContext& context) override;
    
    virtual void AddRenderPasses(RenderGraph& graph, RenderGraphBlackboard& blackboard) override;

private:

	Texture mBRDFLutTexture;
	Texture mGGXEssLutTexture;
	Texture mGGXEAvgLutTexture;

	ComputePipelineHandle mBRDFLutPipeline;
	ComputePipelineHandle mGGXEssLutPipeline;
	ComputePipelineHandle mGGXEAvgLutPipeline;

};

} // namespace Gleam
