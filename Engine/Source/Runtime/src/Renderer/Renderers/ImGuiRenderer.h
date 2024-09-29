#pragma once
#include "Renderer/Renderer.h"

namespace Gleam {

struct ImGuiPassData
{
	TextureHandle sceneTarget;
	TextureHandle swapchainTarget;
};

using ImGuiView = std::function<void(const ImGuiPassData&)>;

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
