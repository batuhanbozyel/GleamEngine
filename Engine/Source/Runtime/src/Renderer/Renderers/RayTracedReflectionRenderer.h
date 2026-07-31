#pragma once
  #include "Renderer/Renderer.h"
  #include "Renderer/RayTracingScene.h"

  namespace Gleam {

  class Material;

GSTRUCT(RayTracedReflectionSettings, "055FBD6B-2B43-470F-8EBF-6CAFD9BFFAE2",
Serializable, PrettyName("Reflection Settings"))
{
	GFIELD("FF566795-8D38-4535-B71C-5B5D70970199", Serializable, PrettyName("Enable"))
	bool enable = true;

	GFIELD("E2AB0950-7CC0-4B00-95C8-992BE592EA99", Serializable, PrettyName("Roughness Cutoff"))
	float roughnessCutoff = 0.5f;

	GFIELD("9C4E1B87-3D2A-4F16-8E5B-7A0C2D9F4E31", Serializable, PrettyName("Denoise"))
	bool denoise = true;

	GFIELD("5B8D3F42-6A19-4C7E-9D03-1F2E8B6A4C57", Serializable, PrettyName("Temporal Stability"))
	float temporalStability = 0.7f;

	// 4 = one ray per pixel, 2 = diagonal of each quad, 1 = one ray per 2x2 quad
	GFIELD("A17C4E90-52B8-4D3F-9E61-0C8F7A2B5D64", Serializable, PrettyName("Samples Per Quad"))
	uint32_t samplesPerQuad = 4;

	GFIELD("3D9A6F15-B04C-4E82-97A5-6E1B8C0D2F73", Serializable, PrettyName("Temporal Variance Guided Tracing"))
	bool temporalVarianceGuidedTracing = true;

	GFIELD("C85B27E4-1A6D-49F0-83B7-5D4E9C1A60F8", Serializable, PrettyName("Variance Threshold"))
	float varianceThreshold = 0.02f;
};

struct RayTracedReflectionData
{
	TextureHandle reflectionTarget = TextureHandle();
};

class RayTracedReflectionRenderer : public IRenderer
{
public:

	RayTracedReflectionRenderer();

	virtual void OnCreate(const RenderContext& context) override;

	virtual void OnDestroy(const RenderContext& context) override;

	virtual void AddRenderPasses(RenderGraph& graph, RenderGraphBlackboard& blackboard) override;

	virtual void RegisterShadingPipeline(const Material* material) override;

	virtual RenderStage GetStage() const override { return RenderStage::Shadows; }

	const RayTracedReflectionSettings& GetSettings() const { return mSettings; }

	void SetSettings(const RayTracedReflectionSettings& settings);

private:

	RayTracedReflectionSettings mSettings;

	void CreateDenoiserTextures(const Size& size);

	void ReleaseDenoiserTextures();

	uint32_t mFrameIndex = 0;
	bool mPipelineDirty = true;
	bool mFirstFrame = true;
	HitGroupTable mHitGroupTable;

	GraphicsDevice* mDevice = nullptr;
	GPUAllocator*   mAllocator = nullptr;
	RayTracingPipelineHandle mRayTracedReflectionPipeline;
	ComputePipelineHandle    mClassificationPipeline;
	ComputePipelineHandle    mPrepareDispatchArgsPipeline;
	ComputePipelineHandle    mReprojectPipeline;
	ComputePipelineHandle    mPrefilterPipeline;
	ComputePipelineHandle    mResolveTemporalPipeline;
	ComputePipelineHandle    mStoreHistoryPipeline;

	Texture mRadiance[2];
	Texture mVariance[2];
	Texture mSampleCount[2];
	Texture mAverageRadiance[2];
	Texture mNormalHistory[2];
	Texture mRoughnessHistory[2];

	Size mDenoiserSize;
};

  } // namespace Gleam
