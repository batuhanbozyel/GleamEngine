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

class VisibilityClassificationRenderer : public IRenderer
{
public:

	virtual void OnCreate(const RenderContext& context) override;

	virtual void OnDestroy(const RenderContext& context) override;

	virtual void AddRenderPasses(RenderGraph& graph, RenderGraphBlackboard& blackboard) override;

	virtual RenderStage GetStage() const override { return RenderStage::Prepass; }

private:

	GraphicsDevice* mDevice = nullptr;
	ComputePipelineHandle mCountPipeline;
	ComputePipelineHandle mAllocatePipeline;
	ComputePipelineHandle mScatterPipeline;
};

} // namespace Gleam
