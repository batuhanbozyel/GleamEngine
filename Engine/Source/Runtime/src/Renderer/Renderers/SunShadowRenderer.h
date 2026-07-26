#pragma once
#include "Renderer/Renderer.h"
#include "Renderer/RayTracingScene.h"

namespace Gleam {

class Material;

GSTRUCT(ShadowSettings, "6F702E03-FBAE-4C02-9FB5-B37E6FB6659D", Serializable, PrettyName("Shadow Settings"))
{
	GFIELD("555B9393-60DD-4402-88FA-903726A13E84", Serializable, PrettyName("Enable"))
	bool enable = true;

	// 0 = unbounded, clamped to the camera far plane
	GFIELD("53F5BC97-EF9D-41F2-BD22-C7DFDD0ABD42", Serializable, PrettyName("Max Ray Distance"))
	float maxRayDistance = 0.0f;
};

struct SunShadowData
{
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

	virtual RenderStage GetStage() const override { return RenderStage::Shadows; }

	const ShadowSettings& GetSettings() const { return mSettings; }

	void SetSettings(const ShadowSettings& settings);

private:

	ShadowSettings mSettings;

	void CreateDenoiserTextures(const Size& size);

	uint32_t mFrameIndex = 0;
	bool mPipelineDirty = true;
	bool mFirstFrame = true;
	HitGroupTable mHitGroupTable;

	GraphicsDevice* mDevice = nullptr;
	GPUAllocator*   mAllocator = nullptr;
	RayTracingPipelineHandle mRayTracedShadowPipeline;
	ComputePipelineHandle    mClassificationPipeline;
	ComputePipelineHandle    mPrepareDispatchArgsPipeline;
	ComputePipelineHandle    mTileClassificationPipeline;
	ComputePipelineHandle    mFilterPipeline;

	Texture mMoments[2];
	Texture mScratch[2];

	Size mDenoiserSize;

};

} // namespace Gleam
