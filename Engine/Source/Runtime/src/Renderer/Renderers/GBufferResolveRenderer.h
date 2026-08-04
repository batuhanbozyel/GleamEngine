#pragma once
#include "Renderer/Renderer.h"

namespace Gleam {

class Material;

struct GBufferData
{
	TextureHandle motionVectorTarget = TextureHandle();
	TextureHandle geometryNormalTarget = TextureHandle();
	TextureHandle shadingNormalTarget = TextureHandle();
	TextureHandle roughnessTarget = TextureHandle();
	TextureHandle barycentricCoordsTarget = TextureHandle();
	TextureHandle barycentricDerivsTarget = TextureHandle();
	TextureHandle previousGeometryNormalTarget = TextureHandle();
	TextureHandle previousShadingNormalTarget = TextureHandle();
	TextureHandle previousRoughnessTarget = TextureHandle();
};

class GBufferResolveRenderer : public IRenderer
{
public:

	virtual void OnCreate(const RenderContext& context) override;

	virtual void OnDestroy(const RenderContext& context) override;

	virtual void AddRenderPasses(RenderGraph& graph, RenderGraphBlackboard& blackboard) override;

	virtual void RegisterShadingPipeline(const Material* material) override;

	virtual RenderStage GetStage() const override { return RenderStage::Prepass; }

private:

	void CreateGBufferTargets(const Size& size);

	void ReleaseGBufferTargets();

	GraphicsDevice* mDevice = nullptr;
	GPUAllocator*   mAllocator = nullptr;
	HashMap<uint32_t, ComputePipelineHandle> mResolvePipelines;

	Texture mGeometryNormalTargets[2];
	Texture mShadingNormalTargets[2];
	Texture mRoughnessTargets[2];
	Size mGBufferSize;
	uint32_t mFrameIndex = 0;
};

} // namespace Gleam
