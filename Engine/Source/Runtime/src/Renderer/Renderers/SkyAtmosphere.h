#pragma once
#include "Renderer/Renderer.h"

namespace Gleam {

struct Atmosphere;

GCLASS(SkyAtmosphereLutRenderer, "2CC0F32E-177F-4474-B29F-A8FDC8EE70E9", Renderer(RenderStage::BeforeRendering, true), PrettyName("Sky Atmosphere LUT")) : public IRenderer
{
public:

	virtual void OnCreate(const RenderContext& context) override;

	virtual void OnDestroy(const RenderContext& context) override;

	virtual void AddRenderPasses(RenderGraph& graph, RenderGraphBlackboard& blackboard) override;

	SkyAtmosphereParameters GetSkyAtmosphereParameters(const Atmosphere& atmosphere) const;

private:

	Texture mTransmittanceLutTexture;
	Texture mMultiScatterLutTexture;

	ComputePipelineHandle mTransmittanceLutPipeline;
	ComputePipelineHandle mMultiScatterLutPipeline;

	SkyAtmosphereParameters mAtmosphereParams = {};
	BarrierLayout mLutLayout = BarrierLayout::Undefined;
};

GCLASS(SkyAtmosphereRenderer, "29526612-1350-4802-A2B9-C3B0C0DF4BB2", Renderer(RenderStage::Sky), PrettyName("Sky Atmosphere")) : public IRenderer
{
public:

	virtual void OnCreate(const RenderContext& context) override;

	virtual void AddRenderPasses(RenderGraph& graph, RenderGraphBlackboard& blackboard) override;

private:

	ComputePipelineHandle mSkyRenderPipeline;
};

} // namespace Gleam
