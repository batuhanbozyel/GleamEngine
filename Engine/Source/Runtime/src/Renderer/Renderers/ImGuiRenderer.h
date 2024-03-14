#pragma once
#include "Renderer/Renderer.h"

namespace Gleam {

using ImGuiView = std::function<void()>;

class ImGuiRenderer : public IRenderer
{
public:
    
    virtual void OnCreate(GraphicsDevice* device) override;

	virtual void OnDestroy(GraphicsDevice* device) override;

	virtual void AddRenderPasses(RenderGraph& graph, RenderGraphBlackboard& blackboard) override;

	void PushView(ImGuiView&& view);
    
private:
    
    GraphicsDevice* mDevice;

	TArray<ImGuiView> mViews;
    
};

} // namespace Gleam
