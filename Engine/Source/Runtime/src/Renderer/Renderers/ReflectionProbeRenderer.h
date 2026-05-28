#pragma once
#include "Renderer/Renderer.h"

namespace Gleam {

struct ReflectionProbePassData
{
	TextureHandle specularReflection = TextureHandle();
	TextureHandle diffuseReflection = TextureHandle();
};

class ReflectionProbeRenderer : public IRenderer
{
public:

	virtual void OnCreate(const RenderContext& context) override;

	virtual void OnDestroy(const RenderContext& context) override;

	virtual void AddRenderPasses(RenderGraph& graph, RenderGraphBlackboard& blackboard) override;

	virtual RenderStage GetStage() const override { return RenderStage::BeforeRendering; }

private:

	CameraUniforms CreateCubeFaceCamera(const float3& position, uint32_t resolution, uint32_t faceIndex);

	ComputePipelineHandle mSkyRenderPipeline;
	ComputePipelineHandle mGenerateMipsPipeline;
	ComputePipelineHandle mDiffuseConvolutionPipeline;
	ComputePipelineHandle mSpecularConvolutionPipeline;
};

} // namespace Gleam