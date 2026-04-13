#pragma once
#include "Renderer/Renderer.h"
#include "Renderer/RayTracingScene.h"

namespace Gleam {

class Material;

GSTRUCT(PathTracerSettings, "A3F7C2D1-8B4E-4F90-BC15-2E6D94A17F83", Serializable)
{
	GFIELD("B8E21C4D-5A93-4D67-9F02-3C7B1E58A6D4", Serializable, PrettyName("Samples Per Pixel"))
	uint32_t samplesPerPixel = 1;

	GFIELD("C9D34F7A-6B04-4E81-A213-4D8C2F69B7E5", Serializable, PrettyName("Max Ray Recursion Depth"))
	uint32_t maxRayRecursionDepth = 8;
};

class PathTracer : public IRenderer
{
public:

	PathTracer();

	virtual void OnCreate(const RenderContext& context) override;

	virtual void OnDestroy(const RenderContext& context) override;

	virtual void AddRenderPasses(RenderGraph& graph, RenderGraphBlackboard& blackboard) override;

	void RegisterShadingPipeline(const Material* material);

	const PathTracerSettings& GetSettings() const { return mSettings; }

	void SetSettings(const PathTracerSettings& settings);

private:

	struct State
	{
		float4x4 cameraView = {};
		SkyAtmosphereUniforms atmosphereUniforms = {};
		SkyAtmosphereParameters atmosphereParams = {};
	} mState;

	bool mPipelineDirty = true;
	HitGroupTable mHitGroupTable;

	PathTracerSettings mSettings;

	Texture mRenderTarget;
	uint32_t mFrameIndex = 0;
	GraphicsDevice* mDevice = nullptr;
	GPUAllocator* mAllocator = nullptr;
	RayTracingPipelineHandle mPathTracingPipeline;

};

} // namespace Gleam
