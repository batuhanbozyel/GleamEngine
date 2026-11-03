#pragma once
#include "Renderer/Renderer.h"

namespace Gleam {

class Material;

struct VisibilityClassificationData
{
	BufferHandle pixelListBuffer = BufferHandle();
	BufferHandle offsetsBuffer = BufferHandle();
	BufferHandle countsBuffer = BufferHandle();
	BufferHandle dispatchArgsBuffer = BufferHandle();
};

GCLASS(VisibilityClassificationRenderer, "D3B96D03-4CD7-4AB3-A350-3DADDF377A08", Renderer(RenderStage::Prepass), PrettyName("Visibility Classification")) : public IRenderer
{
public:

	virtual void OnCreate(const RenderContext& context) override;

	virtual void OnDestroy(const RenderContext& context) override;

	virtual void AddRenderPasses(RenderGraph& graph, RenderGraphBlackboard& blackboard) override;

private:

	GraphicsDevice* mDevice = nullptr;
	ComputePipelineHandle mCountPipeline;
	ComputePipelineHandle mAllocatePipeline;
	ComputePipelineHandle mScatterPipeline;
};

} // namespace Gleam
