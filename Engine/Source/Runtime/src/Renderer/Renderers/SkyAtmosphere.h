#pragma once
#include "Renderer/Renderer.h"

namespace Gleam {

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
	ComputePipelineHandle mTransmittanceLutPipeline;
};

} // namespace Gleam
