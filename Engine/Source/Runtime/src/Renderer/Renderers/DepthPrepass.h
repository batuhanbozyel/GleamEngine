#pragma once
#include "Renderer/Renderer.h"

namespace Gleam {

class Material;

struct DepthPrepassData
{
	TextureHandle depthTarget;
};

class DepthPrepass : public IRenderer
{
public:
    
    virtual void OnCreate(const RenderContext& context) override;

	virtual void OnDestroy(const RenderContext& context) override;
    
    virtual void AddRenderPasses(RenderGraph& graph, RenderGraphBlackboard& blackboard) override;

	void RegisterShadingPipeline(const Material* material);

private:

	GraphicsDevice* mDevice = nullptr;
	HashMap<uint32_t, GraphicsPipelineHandle> mPipelines;

};

} // namespace Gleam
