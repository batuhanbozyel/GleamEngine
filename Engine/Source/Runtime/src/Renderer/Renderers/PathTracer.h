#pragma once
#include "Renderer/Renderer.h"

namespace Gleam {

struct MaterialDescriptor;

class PathTracer : public IRenderer
{
public:
    
    virtual void OnCreate(RenderContext& context) override;

	virtual void OnDestroy(RenderContext& context) override;
    
    virtual void AddRenderPasses(RenderGraph& graph, RenderGraphBlackboard& blackboard) override;

	void RegisterShadingPipeline(const MaterialDescriptor& material, uint32_t hash);

private:

	struct State
	{
		float4x4 cameraView = {};
		SkyAtmosphereUniforms atmosphereUniforms = {};
		SkyAtmosphereParameters atmosphereParams = {};
	} mState;

	Texture mRenderTarget;
	uint32_t mFrameIndex = 0;
	GraphicsDevice* mDevice = nullptr;
	GPUAllocator* mAllocator = nullptr;
	RayTracingPipelineHandle mPathTracingPipeline;

};

} // namespace Gleam
