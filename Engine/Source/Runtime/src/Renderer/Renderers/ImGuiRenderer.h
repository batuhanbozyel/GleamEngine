#pragma once
#include "IO/Path.h"
#include "Renderer/Renderer.h"
#include "Renderer/Texture2D.h"

#include <imgui.h>
#include <imgui_internal.h>

namespace Gleam {

class Swapchain;

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

	ImTextureID GetImTextureIDForTexture(const Texture& texture) const;

	void AddFontTexture(const Path& fontPath, const Path& defaultPath, float fontSize);
    
private:

	Swapchain* mSurface;
    
    GraphicsDevice* mDevice;

	ResourceReleaseQueue* mReleaseQueue;

	GraphicsPipelineHandle mPipeline;

	Heap mHeap;

	Buffer mBuffer;

	Texture2D* mFontTexture = nullptr;

	TArray<ImGuiView> mViews;
    
};

} // namespace Gleam
