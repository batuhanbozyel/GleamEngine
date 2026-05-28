#pragma once
#include "Renderer/Renderer.h"

namespace GEditor {

class InfiniteGridRenderer final : public Gleam::IRenderer
{
public:

    virtual void OnCreate(const Gleam::RenderContext& context) override;
    
    virtual void AddRenderPasses(Gleam::RenderGraph& graph, Gleam::RenderGraphBlackboard& blackboard) override;

    virtual Gleam::RenderStage GetStage() const override { return Gleam::RenderStage::Transparent; }

private:
    
    Gleam::GraphicsPipelineHandle mPipeline;

};

} // namespace Gleam

