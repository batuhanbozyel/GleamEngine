#pragma once
#include "Renderer/Renderer.h"

namespace Gleam {

struct Atmosphere;

class SkyAtmosphereRenderer : public IRenderer
{
public:

	virtual void OnCreate(const RenderContext& context) override;

	virtual void OnDestroy(const RenderContext& context) override;

	virtual void AddRenderPasses(RenderGraph& graph, RenderGraphBlackboard& blackboard) override;

	void UpdateSkyAtmosphere(RenderGraph& graph, RenderGraphBlackboard& blackboard);

	const Texture& GetTransmittanceLutTexture() const
	{
		return mTransmittanceLutTexture;
	}

	const Texture& GetMultiScatterLutTexture() const
	{
		return mMultiScatterLutTexture;
	}

	SkyAtmosphereParameters GetSkyAtmosphereParameters(const Atmosphere& atmosphere) const;

private:

	Texture mTransmittanceLutTexture;
	Texture mMultiScatterLutTexture;

	ComputePipelineHandle mTransmittanceLutPipeline;
	ComputePipelineHandle mMultiScatterLutPipeline;
	ComputePipelineHandle mSkyRenderPipeline;
};

} // namespace Gleam
