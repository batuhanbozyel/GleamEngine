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

	ComputePipelineHandle mPathTracingPipeline;

};

} // namespace Gleam
