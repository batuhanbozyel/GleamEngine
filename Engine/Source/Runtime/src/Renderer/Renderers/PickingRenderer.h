#pragma once
#include "Renderer/Renderer.h"

namespace Gleam {

class Swapchain;
class PickingSystem;

class PickingRenderer final : public IRenderer
{
public:

	PickingRenderer(PickingSystem* system)
		: mSystem(system)
	{
	}

	virtual void OnCreate(const RenderContext& context) override;

	virtual void OnDestroy(const RenderContext& context) override;

	virtual void AddRenderPasses(RenderGraph& graph, RenderGraphBlackboard& blackboard) override;

	virtual RenderStage GetStage() const override { return RenderStage::Prepass; }

private:

	Swapchain* mSwapchain = nullptr;
	PickingSystem* mSystem = nullptr;
	ComputePipelineHandle mPipeline;

};

} // namespace Gleam
