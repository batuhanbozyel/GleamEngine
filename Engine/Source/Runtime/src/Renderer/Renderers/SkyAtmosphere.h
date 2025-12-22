#pragma once
#include "Renderer/Renderer.h"

namespace Gleam {

struct Atmosphere;

struct SkyAtmospherePassData
{
	TextureHandle sceneColor;
	TextureHandle sceneDepth;
	TextureHandle multiScatterLut;
};

class SkyAtmosphereRenderer : public IRenderer
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

	SkyAtmosphereParameters GetSkyAtmosphereParameters(const Atmosphere& atmosphere) const;

	SkyAtmosphereParameters mAtmosphereParams = {};

	Texture mTransmittanceLutTexture;
	Texture mMultiScatterLutTexture;

	ComputePipelineHandle mTransmittanceLutPipeline;
	ComputePipelineHandle mMultiScatterLutPipeline;
	ComputePipelineHandle mSkyRenderPipeline;
};

} // namespace Gleam
