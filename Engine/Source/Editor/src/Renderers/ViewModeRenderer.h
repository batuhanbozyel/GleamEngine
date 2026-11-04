#pragma once
#include "Renderer/Renderer.h"
#include "ViewMode.h"

namespace GEditor {

class ViewModeRenderer final : public Gleam::IRenderer
{
public:

    virtual void OnCreate(const Gleam::RenderContext& context) override;

    virtual void AddRenderPasses(Gleam::RenderGraph& graph, Gleam::RenderGraphBlackboard& blackboard) override;

    void SetViewMode(ViewMode mode) { mMode = mode; }

    ViewMode GetViewMode() const { return mMode; }

private:

    Gleam::GraphicsDevice* mDevice = nullptr;

    // Color format is filled in per view, from the view output
    Gleam::GraphicsPipelineStateDescriptor mPipelineState;

    ViewMode mMode = ViewMode::Lit;

};

} // namespace GEditor
