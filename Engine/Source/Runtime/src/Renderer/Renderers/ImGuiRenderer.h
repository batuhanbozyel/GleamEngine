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
    
    virtual void OnCreate(const RenderContext& context) override;

	virtual void OnDestroy(const RenderContext& context) override;

	virtual void AddRenderPasses(RenderGraph& graph, RenderGraphBlackboard& blackboard) override;

	void PushView(ImGuiView&& view);

	ImTextureID GetImTextureIDForTexture(const Texture& texture) const;

	void AddFontTexture(const Path& fontPath, const Path& defaultPath, float fontSize);
    
private:

	Swapchain* mSurface;

	ResourceReleaseQueue* mReleaseQueue;

	GraphicsPipelineHandle mPipeline;

	Buffer mBuffer;

	Texture2D* mFontTexture = nullptr;

	Texture2D* mDefaultFontTexture = nullptr;

	TArray<ImGuiView> mViews;
    
};

} // namespace Gleam
