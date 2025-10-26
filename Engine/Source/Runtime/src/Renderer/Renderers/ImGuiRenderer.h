#pragma once
#include "Renderer/Renderer.h"

namespace Gleam {

struct ImGuiPassData
{
	TextureHandle sceneTarget;
	TextureHandle backbuffer;
};

using ImGuiView = std::function<void(const ImGuiPassData&)>;

class ImGuiRenderer : public IRenderer
{
public:
    
    virtual void OnCreate(RenderContext& context) override;

	virtual void OnDestroy(RenderContext& context) override;

	virtual void AddRenderPasses(RenderGraph& graph, RenderGraphBlackboard& blackboard) override;

	void PushView(ImGuiView&& view);
    
private:

	RenderSurface* mSurface;
    
    GraphicsDevice* mDevice;

	TArray<ImGuiView> mViews;
    
};

} // namespace Gleam
