#pragma once
#include "Renderer/Renderer.h"
#include "Renderer/RayTracingScene.h"

namespace Gleam {

class Material;

class PathTracer : public IRenderer
{
public:

	PathTracer();
    
    virtual void OnCreate(RenderContext& context) override;

	virtual void OnDestroy(RenderContext& context) override;
    
    virtual void AddRenderPasses(RenderGraph& graph, RenderGraphBlackboard& blackboard) override;

	void RegisterShadingPipeline(const Material* material);

private:

	struct State
	{
		float4x4 cameraView = {};
		SkyAtmosphereUniforms atmosphereUniforms = {};
		SkyAtmosphereParameters atmosphereParams = {};
	} mState;

	bool mPipelineDirty = true;
	HitGroupTable mHitGroupTable;

	Texture mRenderTarget;
	uint32_t mFrameIndex = 0;
	uint32_t mMaxRayRecursionDepth = 16;
	GraphicsDevice* mDevice = nullptr;
	GPUAllocator* mAllocator = nullptr;
	RayTracingPipelineHandle mPathTracingPipeline;

};

} // namespace Gleam
