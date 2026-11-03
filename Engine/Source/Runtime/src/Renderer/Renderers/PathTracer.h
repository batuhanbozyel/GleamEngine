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

struct PathTracerHistory
{
	Texture accumulation;
	float4x4 cameraView = {};
	SkyAtmosphereUniforms atmosphereUniforms = {};
	SkyAtmosphereParameters atmosphereParams = {};
	uint32_t frameIndex = 0;
	uint32_t version = 0;

	void Release(GraphicsDevice* device, GPUAllocator* allocator);
};

GCLASS(PathTracer, "5039B50A-228B-4A56-833A-B4DF0EF28C1D", Renderer(RenderStage::Opaque), PrettyName("Path Tracer")) : public IRenderer
{
public:

	PathTracer();

	virtual void OnCreate(const RenderContext& context) override;

	virtual void AddRenderPasses(RenderGraph& graph, RenderGraphBlackboard& blackboard) override;

	virtual void RegisterShadingPipeline(const Material* material) override;

	const PathTracerSettings& GetSettings() const { return mSettings; }

	void SetSettings(const PathTracerSettings& settings);

private:

	bool mPipelineDirty = true;
	uint32_t mHistoryVersion = 0;
	HitGroupTable mHitGroupTable;

	PathTracerSettings mSettings;

	GraphicsDevice* mDevice = nullptr;
	RayTracingPipelineHandle mPathTracingPipeline;

};

} // namespace Gleam
