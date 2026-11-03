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

	GFIELD("20C9DB3A-A905-4EC3-A480-90348612B776", Serializable, PrettyName("Denoise"))
	bool denoise = true;
};

struct SunShadowData
{
	TextureHandle shadowMask;
};

struct SunShadowHistory
{
	Texture moments;
	Texture shadow;
	uint32_t frameIndex = 0;
	uint32_t version = 0;

	void Release(GraphicsDevice* device, GPUAllocator* allocator);
};

GCLASS(SunShadowRenderer, "0B9A9ABD-70E4-4AFF-B603-745BAC2E675E", Renderer(RenderStage::Shadows), PrettyName("Sun Shadows")) : public IRenderer
{
public:

	SunShadowRenderer();

	virtual void OnCreate(const RenderContext& context) override;

	virtual void AddRenderPasses(RenderGraph& graph, RenderGraphBlackboard& blackboard) override;

	virtual void RegisterShadingPipeline(const Material* material) override;

	const ShadowSettings& GetSettings() const { return mSettings; }

	void SetSettings(const ShadowSettings& settings);

private:

	ShadowSettings mSettings;

	bool mPipelineDirty = true;
	uint32_t mHistoryVersion = 0;
	HitGroupTable mHitGroupTable;

	GraphicsDevice* mDevice = nullptr;
	RayTracingPipelineHandle mRayTracedShadowPipeline;
	ComputePipelineHandle    mClassificationPipeline;
	ComputePipelineHandle    mPrepareDispatchArgsPipeline;
	ComputePipelineHandle    mTileClassificationPipeline;
	ComputePipelineHandle    mFilterPipeline;
	ComputePipelineHandle    mResolvePipeline;

};

} // namespace Gleam
