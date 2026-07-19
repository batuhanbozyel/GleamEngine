#pragma once
#include "Renderer/Renderer.h"

namespace Gleam {

struct AmbientOcclusionData
{
	TextureHandle ambientOcclusionTexture = TextureHandle();
};

class AmbientOcclusionRenderer : public IRenderer
{
public:

	virtual void OnCreate(const RenderContext& context) override;

	virtual void AddRenderPasses(RenderGraph& graph, RenderGraphBlackboard& blackboard) override;

	virtual RenderStage GetStage() const override { return RenderStage::Shadows; }

private:

	GraphicsDevice* mDevice = nullptr;
	uint32_t mFrameIndex = 0;

	ComputePipelineHandle mDepthPrefilterPipeline;
	ComputePipelineHandle mMainPassPipeline;
	ComputePipelineHandle mDenoisePipeline;

};

} // namespace Gleam
