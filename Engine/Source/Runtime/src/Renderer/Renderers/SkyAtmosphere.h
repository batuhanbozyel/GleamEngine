#pragma once
#include "Renderer/Renderer.h"

namespace Gleam {

struct SkyAtmospherePassData
{
	TextureHandle sceneColor;
	TextureHandle sceneDepth;
	TextureHandle multiScatterLut;
};

class SkyAtmosphere : public IRenderer
{
public:

	virtual void OnCreate(RenderContext& context) override;

	virtual void OnDestroy(RenderContext& context) override;

	virtual void AddRenderPasses(RenderGraph& graph, RenderGraphBlackboard& blackboard) override;

	ShaderResourceIndex GetTransmittanceLutTexture() const
	{
		return mTransmittanceLutTexture.GetResourceView();
	}

	ShaderResourceIndex GetMultiScatterLutTexture() const
	{
		return mMultiScatterLutTexture.GetResourceView();
	}

private:

	bool mBakeLUTs = true;
	SkyAtmosphereParameters mAtmosphereParams = {};

	Texture mTransmittanceLutTexture;
	Texture mMultiScatterLutTexture;

	ComputePipelineHandle mTransmittanceLutPipeline;
	ComputePipelineHandle mMultiScatterLutPipeline;
	ComputePipelineHandle mSkyRenderPipeline;
};

} // namespace Gleam
