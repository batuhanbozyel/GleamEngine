#pragma once
#include "Renderer/Renderer.h"

namespace Gleam {

class Material;

struct GBufferData
{
	TextureHandle motionVectorTarget = TextureHandle();
	TextureHandle geometryNormalTarget = TextureHandle();
	TextureHandle shadingNormalTarget = TextureHandle();
	TextureHandle roughnessTarget = TextureHandle();
	TextureHandle barycentricCoordsTarget = TextureHandle();
	TextureHandle barycentricDerivsTarget = TextureHandle();
	TextureHandle previousGeometryNormalTarget = TextureHandle();
	TextureHandle previousShadingNormalTarget = TextureHandle();
	TextureHandle previousRoughnessTarget = TextureHandle();
};

struct GBufferResolveHistory
{
	Texture geometryNormal[2];
	Texture shadingNormal[2];
	Texture roughness[2];
	uint32_t frameIndex = 0;
	bool firstFrame = true;

	void Release(GraphicsDevice* device, GPUAllocator* allocator);
};

GCLASS(GBufferResolveRenderer, "6BABE54F-AF07-415A-A8AD-4F3A31E44B90", Renderer(RenderStage::Prepass), PrettyName("G-Buffer Resolve")) : public IRenderer
{
public:

	virtual void OnCreate(const RenderContext& context) override;

	virtual void AddRenderPasses(RenderGraph& graph, RenderGraphBlackboard& blackboard) override;

	virtual void RegisterShadingPipeline(const Material* material) override;

private:

	void CreateGBufferTargets(GBufferResolveHistory& history, GPUAllocator* allocator, const Size& size) const;

	GraphicsDevice* mDevice = nullptr;
	HashMap<uint32_t, ComputePipelineHandle> mResolvePipelines;
};

} // namespace Gleam
