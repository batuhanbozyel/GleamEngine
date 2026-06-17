#pragma once
#include "Renderer/Renderer.h"

namespace Gleam {

class Material;

struct DepthPrepassData
{
	TextureHandle depthTarget;
	TextureHandle motionVectorTarget;
	TextureHandle normalTarget;
};

class DepthPrepass : public IRenderer
{
public:
    
    virtual void OnCreate(const RenderContext& context) override;

	virtual void OnDestroy(const RenderContext& context) override;
    
    virtual void AddRenderPasses(RenderGraph& graph, RenderGraphBlackboard& blackboard) override;

	virtual void RegisterShadingPipeline(const Material* material) override;

	virtual RenderStage GetStage() const override { return RenderStage::Prepass; }

private:

	GraphicsDevice* mDevice = nullptr;
	HashMap<uint32_t, GraphicsPipelineHandle> mPipelines;

};

} // namespace Gleam
