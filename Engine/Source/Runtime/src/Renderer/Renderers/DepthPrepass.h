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

GCLASS(DepthPrepass, "6E301423-A496-4A1B-9042-5AA4FB6BE061", Renderer(RenderStage::Prepass), PrettyName("Depth Prepass")) : public IRenderer
{
public:
    
    virtual void OnCreate(const RenderContext& context) override;

	virtual void OnDestroy(const RenderContext& context) override;
    
    virtual void AddRenderPasses(RenderGraph& graph, RenderGraphBlackboard& blackboard) override;

	virtual void RegisterShadingPipeline(const Material* material) override;

private:

	void CreateDepthBuffers(const Size& size);

	GraphicsDevice* mDevice = nullptr;
	GPUAllocator*   mAllocator = nullptr;
	HashMap<uint32_t, MeshPipelineHandle> mPipelines;

	Texture mDepthBuffers[2];
	Size mDepthBufferSize;
	uint32_t mFrameIndex = 0;
	bool mFirstFrame = true;

};

} // namespace Gleam
