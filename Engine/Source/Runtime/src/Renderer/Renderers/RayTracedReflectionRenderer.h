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

	virtual void AddRenderPasses(RenderGraph& graph, RenderGraphBlackboard& blackboard) override;

	virtual void RegisterShadingPipeline(const Material* material) override;

	virtual RenderStage GetStage() const override { return RenderStage::Shadows; }

	const RayTracedReflectionSettings& GetSettings() const { return mSettings; }

	void SetSettings(const RayTracedReflectionSettings& settings);

private:

	RayTracedReflectionSettings mSettings;
	uint32_t mFrameIndex = 0;
	bool mPipelineDirty = true;
	HitGroupTable mHitGroupTable;

	GraphicsDevice* mDevice = nullptr;
	RayTracingPipelineHandle mRayTracedReflectionPipeline;
};

  } // namespace Gleam