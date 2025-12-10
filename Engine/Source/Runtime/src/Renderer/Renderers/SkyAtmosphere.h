#pragma once
#include "Renderer/Renderer.h"

namespace Gleam {

class SkyAtmosphereRenderer : public IRenderer
{
public:

	virtual void OnCreate(RenderContext& context) override;

	virtual void AddRenderPasses(RenderGraph& graph, RenderGraphBlackboard& blackboard) override;

private:

	ComputePipelineHandle mTransmittanceLutPipeline;
};

} // namespace Gleam
