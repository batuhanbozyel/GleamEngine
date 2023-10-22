#pragma once
#include "Gleam.h"

namespace GEditor {

class ImGuiRenderer : public Gleam::IRenderer
{
public:
    
    virtual void OnCreate(Gleam::GraphicsDevice* device) override;

	virtual void OnDestroy(Gleam::GraphicsDevice* device) override;

	virtual void AddRenderPasses(Gleam::RenderGraph& graph, Gleam::RenderGraphBlackboard& blackboard) override;
    
private:
    
    Gleam::GraphicsDevice* mDevice;
    
};

} // namespace GEditor
