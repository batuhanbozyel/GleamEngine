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

struct RayTracedReflectionHistory
{
	Texture radiance[2];
	Texture variance[2];
	Texture sampleCount[2];
	Texture averageRadiance;
	uint32_t frameIndex = 0;
	uint32_t version = 0;
	bool firstFrame = true;

	void ReleasePersistentTextures(GraphicsDevice* device, GPUAllocator* allocator);

	void ReleaseDenoiserTextures(GraphicsDevice* device, GPUAllocator* allocator);

	void Release(GraphicsDevice* device, GPUAllocator* allocator);
};

GCLASS(RayTracedReflectionRenderer, "F2ADDFF5-ECB9-46B5-AFCB-D367F64CA6A9", Renderer(RenderStage::Shadows), PrettyName("Ray Traced Reflections")) : public IRenderer
{
public:

	RayTracedReflectionRenderer();

	virtual void OnCreate(const RenderContext& context) override;

	virtual void AddRenderPasses(RenderGraph& graph, RenderGraphBlackboard& blackboard) override;

	virtual void RegisterShadingPipeline(const Material* material) override;

	const RayTracedReflectionSettings& GetSettings() const { return mSettings; }

	void SetSettings(const RayTracedReflectionSettings& settings);

private:

	RayTracedReflectionSettings mSettings;

	void CreatePersistentTextures(RayTracedReflectionHistory& history, GPUAllocator* allocator, const Size& size) const;

	void CreateDenoiserTextures(RayTracedReflectionHistory& history, GPUAllocator* allocator, const Size& size) const;

	bool mPipelineDirty = true;
	uint32_t mHistoryVersion = 0;
	HitGroupTable mHitGroupTable;

	GraphicsDevice* mDevice = nullptr;
	RayTracingPipelineHandle mRayTracedReflectionPipeline;
	ComputePipelineHandle    mClassificationPipeline;
	ComputePipelineHandle    mPrepareDispatchArgsPipeline;
	ComputePipelineHandle    mReprojectPipeline;
	ComputePipelineHandle    mPrefilterPipeline;
	ComputePipelineHandle    mResolveTemporalPipeline;
};

  } // namespace Gleam
