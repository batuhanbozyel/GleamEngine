#pragma once
#include "Renderer/Renderer.h"
#include "Renderer/RayTracingScene.h"

namespace Gleam {

class Material;

struct SunShadowData
{
	TextureHandle depthTarget;
	TextureHandle shadowMask;
};

class SunShadowRenderer : public IRenderer
{
public:

	SunShadowRenderer();

	virtual void OnCreate(const RenderContext& context) override;

	virtual void OnDestroy(const RenderContext& context) override;

	virtual void AddRenderPasses(RenderGraph& graph, RenderGraphBlackboard& blackboard) override;

	virtual void RegisterShadingPipeline(const Material* material) override;

private:

	void CreateDenoiserTextures(const Size& size);

	uint32_t mFrameIndex = 0;
	bool mPipelineDirty = true;
	bool mFirstFrame = true;
	HitGroupTable mHitGroupTable;

	GraphicsDevice* mDevice = nullptr;
	GPUAllocator*   mAllocator = nullptr;
	RayTracingPipelineHandle mRayTracedShadowPipeline;
	ComputePipelineHandle    mTileClassificationPipeline;
	ComputePipelineHandle    mFilterPipeline;
	ComputePipelineHandle    mDepthCopyPipeline;

	// Persistent temporal resources (ping-pong on mFrameIndex)
	Texture mMoments[2];         // R16G16B16A16_SFloat — mean, variance, sample count
	Texture mHistoryShadow[2];   // R16G16_SFloat       — denoised shadow history
	Texture mPreviousDepth;      // R32_SFloat          — depth from previous frame
	Texture mDenoisedShadow;     // R8G8B8A8_UNorm      — final denoised output

	Size mDenoiserSize;

};

} // namespace Gleam
