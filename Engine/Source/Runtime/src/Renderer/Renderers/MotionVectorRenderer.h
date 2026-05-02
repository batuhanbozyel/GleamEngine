#pragma once
#include "Renderer/Renderer.h"

namespace Gleam {

struct MotionVectorData
{
    TextureHandle motionVectorTarget;
	TextureHandle depthTarget;
};

class MotionVectorRenderer : public IRenderer
{
public:

    virtual void OnCreate(const RenderContext& context) override;

    virtual void OnDestroy(const RenderContext& context) override;

    virtual void AddRenderPasses(RenderGraph& graph, RenderGraphBlackboard& blackboard) override;

private:

    GraphicsDevice* mDevice = nullptr;
    TArray<GraphicsPipelineHandle, 3> mPipelines;

};

} // namespace Gleam
