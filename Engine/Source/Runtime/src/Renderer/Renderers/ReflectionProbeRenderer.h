#pragma once
#include "Renderer/Renderer.h"

namespace Gleam {

class ReflectionProbeRenderer : public IRenderer
{
public:

	virtual void OnCreate(RenderContext& context) override;

	virtual void OnDestroy(RenderContext& context) override;

	virtual void AddRenderPasses(RenderGraph& graph, RenderGraphBlackboard& blackboard) override;

private:


};

} // namespace Gleam