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

	void RegisterShadingPipeline(const Material* material);

private:

	uint32_t mFrameIndex = 0;
	bool mPipelineDirty = true;
	HitGroupTable mHitGroupTable;

	GraphicsDevice* mDevice = nullptr;
	RayTracingPipelineHandle mRayTracedShadowPipeline;

};

} // namespace Gleam
