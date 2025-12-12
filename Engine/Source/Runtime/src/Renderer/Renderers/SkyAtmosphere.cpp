#include "gpch.h"
#include "SkyAtmosphere.h"
#include "Renderer/Shaders/Atmosphere/SkyAtmosphereDefinitions.h"

#include "Renderer/CommandBuffer.h"
#include "Renderer/RenderSurface.h"
#include "Renderer/GraphicsDevice.h"

using namespace Gleam;

void SkyAtmosphere::OnCreate(RenderContext& context)
{
	ComputePipelineStateDescriptor pipelineState;
	pipelineState.entryPoint = "skyAtmosphereTransmittanceLUTShader";
	mTransmittanceLutPipeline = context.device->CreateComputePipeline(pipelineState);

	{
		TextureDescriptor textureDesc;
		textureDesc.name = "SkyAtmosphereTransmittanceLUT";
		textureDesc.dimension = TextureDimension::Texture2D;
		textureDesc.format = TextureFormat::R16G16B16A16_SFloat;
		textureDesc.usage = TextureUsage_Storage | TextureUsage_Sampled;
		textureDesc.size = { SKY_ATMOSPHERE_TRANSMITTANCE_TEXTURE_WIDTH, SKY_ATMOSPHERE_TRANSMITTANCE_TEXTURE_HEIGHT };
		mTransmittanceLutTexture = context.device->CreateTexture(context.allocator, textureDesc);
	}
}

void SkyAtmosphere::OnDestroy(RenderContext& context)
{
	context.device->Dispose(context.allocator, mTransmittanceLutTexture);
}

void SkyAtmosphere::AddRenderPasses(RenderGraph& graph, RenderGraphBlackboard& blackboard)
{
	auto transmittanceLut = graph.ImportTexture(mTransmittanceLutTexture);
	if (mBakeLUTs)
	{
		struct SkyAtmosphereTransmittanceLutPassData
		{
			TextureHandle texture;
		};
		graph.AddRenderPass<SkyAtmosphereTransmittanceLutPassData>("SkyAtmosphere::TransmittanceLut", [&](RenderGraphBuilder& builder, SkyAtmosphereTransmittanceLutPassData& passData)
		{
			passData.texture = builder.WriteTexture(transmittanceLut);
		},
		[this](const CommandBuffer* cmd, const SkyAtmosphereTransmittanceLutPassData& passData)
		{
			cmd->BindComputePipeline(mTransmittanceLutPipeline);
			cmd->Dispatch(SKY_ATMOSPHERE_TRANSMITTANCE_TEXTURE_WIDTH / 16, SKY_ATMOSPHERE_TRANSMITTANCE_TEXTURE_HEIGHT / 16, 1);
		});

		mBakeLUTs = false;
	}
}