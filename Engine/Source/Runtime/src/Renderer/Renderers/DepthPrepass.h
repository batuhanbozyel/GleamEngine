#pragma once
#include "Renderer/Renderer.h"

namespace Gleam {

class Material;

struct DepthPrepassData
{
	TextureHandle depthTarget;
	TextureHandle visibilityBuffer;
	TextureHandle previousDepth;
};

struct DepthPrepassHistory
{
	Texture depth;

	void Release(GraphicsDevice* device, GPUAllocator* allocator);
};

GCLASS(DepthPrepass, "6E301423-A496-4A1B-9042-5AA4FB6BE061", Renderer(RenderStage::Prepass), PrettyName("Depth Prepass")) : public IRenderer
{
public:
    
    virtual void OnCreate(const RenderContext& context) override;

    virtual void AddRenderPasses(RenderGraph& graph, RenderGraphBlackboard& blackboard) override;

	virtual void RegisterShadingPipeline(const Material* material) override;

private:

	GraphicsDevice* mDevice = nullptr;
	HashMap<uint32_t, MeshPipelineHandle> mPipelines;

};

} // namespace Gleam
