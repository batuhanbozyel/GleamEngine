#pragma once
#include "Renderer/Renderer.h"
#include "Renderer/ViewMode.h"

namespace GEditor {

class ViewModeRenderer final : public Gleam::IRenderer
{
public:

    virtual void OnCreate(const Gleam::RenderContext& context) override;

    virtual void AddRenderPasses(Gleam::RenderGraph& graph, Gleam::RenderGraphBlackboard& blackboard) override;

    virtual Gleam::RenderStage GetStage() const override { return Gleam::RenderStage::PostProcess; }

    void SetViewMode(Gleam::ViewMode mode) { mMode = mode; }

    Gleam::ViewMode GetViewMode() const { return mMode; }

private:

    Gleam::GraphicsDevice* mDevice = nullptr;

    Gleam::GraphicsPipelineHandle mPipeline;

    Gleam::ViewMode mMode = Gleam::ViewMode::Lit;

};

} // namespace GEditor
