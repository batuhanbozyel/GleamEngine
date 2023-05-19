#pragma once
#include "Gleam.h"
#include "ImGuiBackend.h"

namespace GEditor {

class ImGuiRenderer : public Gleam::IRenderer
{
public:
    
    virtual void OnCreate(Gleam::RendererContext* context) override;

	virtual void OnDestroy() override;

	virtual void AddRenderPasses(Gleam::RenderGraph& graph, Gleam::RenderGraphBlackboard& blackboard) override;
    
private:
    
    Gleam::Scope<ImGuiBackend> mBackend;
    
};

} // namespace GEditor