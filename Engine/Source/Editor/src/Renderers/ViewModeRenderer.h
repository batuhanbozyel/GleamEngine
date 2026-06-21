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

    void AddMeshletVisualizationPass(Gleam::RenderGraph& graph, Gleam::RenderGraphBlackboard& blackboard);

    Gleam::GraphicsDevice* mDevice = nullptr;

    Gleam::GraphicsPipelineHandle mPipeline;

	// Meshlet visualization pipelines for each culling mode (None, Back, Front)
    Gleam::TArray<Gleam::MeshPipelineHandle, 3> mMeshletVisPipelines;

    Gleam::ViewMode mMode = Gleam::ViewMode::Lit;

};

} // namespace GEditor
