#pragma once
#include "Renderer/Renderer.h"

namespace Gleam {

struct ReflectionProbePassData
{
	TextureHandle specularReflection = TextureHandle();
	TextureHandle diffuseReflection = TextureHandle();
};

GCLASS(ReflectionProbeRenderer, "80BFDA6C-4337-4B03-AD30-42D7950A16D9", Renderer(RenderStage::BeforeRendering), PrettyName("Reflection Probes")) : public IRenderer
{
public:

	virtual void OnCreate(const RenderContext& context) override;

	virtual void OnDestroy(const RenderContext& context) override;

	virtual void AddRenderPasses(RenderGraph& graph, RenderGraphBlackboard& blackboard) override;

private:

	CameraUniforms CreateCubeFaceCamera(const float3& position, uint32_t resolution, uint32_t faceIndex);

	ComputePipelineHandle mSkyRenderPipeline;
	ComputePipelineHandle mGenerateMipsPipeline;
	ComputePipelineHandle mDiffuseConvolutionPipeline;
	ComputePipelineHandle mSpecularConvolutionPipeline;
};

} // namespace Gleam