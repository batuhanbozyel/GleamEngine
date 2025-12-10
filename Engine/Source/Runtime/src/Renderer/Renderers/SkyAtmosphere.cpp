#include "gpch.h"
#include "SkyAtmosphere.h"

#include "Renderer/CommandBuffer.h"
#include "Renderer/RenderSurface.h"
#include "Renderer/GraphicsDevice.h"

using namespace Gleam;

void SkyAtmosphereRenderer::OnCreate(RenderContext& context)
{
	ComputePipelineStateDescriptor pipelineState;
	pipelineState.entryPoint = "skyAtmosphereTransmittanceLUTShader";
	mTransmittanceLutPipeline = context.device->CreateComputePipeline(pipelineState);
}

void SkyAtmosphereRenderer::AddRenderPasses(RenderGraph& graph, RenderGraphBlackboard& blackboard)
{
	
}