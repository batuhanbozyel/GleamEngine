#pragma once
#include "Renderer/Renderer.h"

namespace Gleam {

struct SkyAtmospherePassData
{
	TextureHandle sceneColor;
	TextureHandle sceneDepth;
	TextureHandle multiScatterLut;
	TextureHandle skyViewLut;
	TextureHandle aerialPerspectiveLut;
};

class SkyAtmosphere : public IRenderer
{
public:

	virtual void OnCreate(RenderContext& context) override;

	virtual void OnDestroy(RenderContext& context) override;

	virtual void AddRenderPasses(RenderGraph& graph, RenderGraphBlackboard& blackboard) override;

private:

	bool mBakeLUTs = true;
	SkyAtmosphereParameters mAtmosphereParams = {};

	Texture mTransmittanceLutTexture;
	Texture mMultiScatterLutTexture;
	Texture mSkyViewLutTexture;
	Texture mAerialPerspectiveLutTexture;

	ComputePipelineHandle mTransmittanceLutPipeline;
	ComputePipelineHandle mMultiScatterLutPipeline;
	ComputePipelineHandle mSkyViewLutPipeline;
	ComputePipelineHandle mAerialPerspectiveLutPipeline;
	ComputePipelineHandle mSkyRenderPipeline;
};

} // namespace Gleam
