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

using ImGuiLayout = std::function<void(ImGuiID)>;

class ImGuiRenderer : public IRenderer
{
public:
    
    virtual void OnCreate(const RenderContext& context) override;

	virtual void OnDestroy(const RenderContext& context) override;

	virtual void AddRenderPasses(RenderGraph& graph, RenderGraphBlackboard& blackboard) override;

	virtual RenderStage GetStage() const override { return RenderStage::AfterRendering; }

	void PushView(ImGuiView&& view);

	void ApplyLayout(ImGuiLayout&& layout);

	ImTextureID GetImTextureIDForTexture(const Texture& texture) const;

	void ClearFonts();

	ImFont* AddFont(const Path& fontPath, float fontSize, const ImWchar* excludeRanges = nullptr);

	void MergeFont(const Path& fontPath, float fontSize, const ImWchar* glyphRanges);

	void BuildFontTexture();
    
private:

	Swapchain* mSurface;

	ResourceReleaseQueue* mReleaseQueue;

	GraphicsPipelineHandle mPipeline;

	Buffer mBuffer;

	Texture2D* mFontTexture = nullptr;

	Texture2D* mDefaultFontTexture = nullptr;

	TArray<ImGuiView> mViews;

	ImGuiLayout mPendingLayout;
    
};

} // namespace Gleam
