#pragma once
#include "Renderer/Renderer.h"

namespace Gleam {

GENUM(AmbientOcclusionQuality, "6B168FE1-01D0-4EF5-B2F6-947E467DE4D8", Serializable, PrettyName("Ambient Occlusion Quality"))
{
	GITEM(Low, "5ED1840A-0024-400F-8E17-7129A0FF03E8") = 0,
	GITEM(Medium, "7F6C6690-E08B-4C85-B901-B7AACE5FFCB8") = 1,
	GITEM(High, "F29AC82D-FDF2-453C-836A-8E0999680AC8") = 2,
	GITEM(Ultra, "5FE390FF-E39D-4D40-A2E0-693F6E2D03C6") = 3
};

GENUM(AmbientOcclusionMode, "670BDEF6-6200-45F6-960E-F8785A4414EE", Serializable, PrettyName("Ambient Occlusion Mode"))
{
	GITEM(None, "2202A506-3F57-4B3F-9920-08B29008DE05") = 0,
	GITEM(Sharp, "395C5502-5BF9-4974-B1FC-105E4C2AD604") = 1,
	GITEM(Medium, "A11FA04B-ECAA-4518-93B2-EC53D2AE4883") = 2,
	GITEM(Soft, "6B514F60-4A16-4939-A72F-B1BD1F06744A") = 3
};

GSTRUCT(AmbientOcclusionSettings, "B43BE33D-FCCF-454A-A8CA-0998EA6667EE", Serializable, PrettyName("Ambient Occlusion Settings"))
{
	GFIELD("4B50CD25-BBED-445E-ADD5-AA4C2B6DF379", Serializable, PrettyName("Enable"))
	bool enable = true;

	GFIELD("2471BACF-A0E9-4937-A88C-90C1C3226BE6", Serializable, PrettyName("Quality"))
	AmbientOcclusionQuality quality = AmbientOcclusionQuality::Ultra;

	GFIELD("C0A1D9B5-3E7F-4D8A-BE2B-6C9F1A1D8B2C", Serializable, PrettyName("Mode"))
	AmbientOcclusionMode mode = AmbientOcclusionMode::Sharp;
};

struct AmbientOcclusionData
{
	TextureHandle aoTarget = TextureHandle();
};

class AmbientOcclusionRenderer : public IRenderer
{
public:

	virtual void OnCreate(const RenderContext& context) override;

	virtual void AddRenderPasses(RenderGraph& graph, RenderGraphBlackboard& blackboard) override;

	virtual RenderStage GetStage() const override { return RenderStage::Shadows; }

	const AmbientOcclusionSettings& GetSettings() const { return mSettings; }

	void SetSettings(const AmbientOcclusionSettings& settings);

private:
	
	GTAOConstants SetupGTAOConstants(const CameraUniforms& camera, uint32_t frameIndex) const;

	AmbientOcclusionSettings mSettings;
	GraphicsDevice* mDevice = nullptr;
	uint32_t mFrameIndex = 0;

	ComputePipelineHandle mDepthPrefilterPipeline;
	ComputePipelineHandle mDenoisePipeline;
	TArray<ComputePipelineHandle, static_cast<size_t>(AmbientOcclusionQuality::Ultra) + 1> mMainPassPipelines;

};

} // namespace Gleam
