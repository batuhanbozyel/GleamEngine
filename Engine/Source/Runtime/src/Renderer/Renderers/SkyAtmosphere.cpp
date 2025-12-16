#include "gpch.h"
#include "SkyAtmosphere.h"
#include "Renderer/Shaders/Atmosphere/SkyAtmosphereDefinitions.h"

#include "Renderer/CommandBuffer.h"
#include "Renderer/RenderSurface.h"
#include "Renderer/GraphicsDevice.h"
#include "Renderer/Renderers/WorldRenderer.h"

using namespace Gleam;

void SkyAtmosphere::OnCreate(RenderContext& context)
{
	// Transmittance LUT
	{
		ComputePipelineStateDescriptor pipelineState;
		pipelineState.entryPoint = "skyAtmosphereTransmittanceLUTShader";
		mTransmittanceLutPipeline = context.device->CreateComputePipeline(pipelineState);

		TextureDescriptor textureDesc;
		textureDesc.name = "SkyAtmosphereTransmittanceLUT";
		textureDesc.dimension = TextureDimension::Texture2D;
		textureDesc.format = TextureFormat::R16G16B16A16_SFloat;
		textureDesc.usage = TextureUsage_Storage | TextureUsage_Sampled;
		textureDesc.size = { SKY_ATMOSPHERE_TRANSMITTANCE_TEXTURE_WIDTH, SKY_ATMOSPHERE_TRANSMITTANCE_TEXTURE_HEIGHT };
		mTransmittanceLutTexture = context.device->CreateTexture(context.allocator, textureDesc);
	}

	// Multi scatter LUT
	{
		ComputePipelineStateDescriptor pipelineState;
		pipelineState.entryPoint = "skyAtmosphereMultiScatterLUTShader";
		mMultiScatterLutPipeline = context.device->CreateComputePipeline(pipelineState);

		TextureDescriptor textureDesc;
		textureDesc.name = "SkyAtmosphereMultiScatterLUT";
		textureDesc.dimension = TextureDimension::Texture2D;
		textureDesc.format = TextureFormat::R16G16B16A16_SFloat;
		textureDesc.usage = TextureUsage_Storage | TextureUsage_Sampled;
		textureDesc.size = { SKY_ATMOSPHERE_MULTISCATTERING_LUT_RES, SKY_ATMOSPHERE_MULTISCATTERING_LUT_RES };
		mMultiScatterLutTexture = context.device->CreateTexture(context.allocator, textureDesc);
	}

	// Sky view LUT
	{
		ComputePipelineStateDescriptor pipelineState;
		pipelineState.entryPoint = "skyAtmosphereSkyViewLUTShader";
		mSkyViewLutPipeline = context.device->CreateComputePipeline(pipelineState);

		TextureDescriptor textureDesc;
		textureDesc.name = "SkyAtmosphereSkyViewLUT";
		textureDesc.dimension = TextureDimension::Texture2D;
		textureDesc.format = TextureFormat::R11G11B10_SFloat;
		textureDesc.usage = TextureUsage_Storage | TextureUsage_Sampled;
		textureDesc.size = { SKY_ATMOSPHERE_SKY_VIEW_TEXTURE_WIDTH, SKY_ATMOSPHERE_SKY_VIEW_TEXTURE_HEIGHT };
		mSkyViewLutTexture = context.device->CreateTexture(context.allocator, textureDesc);
	}

	// Setup atmosphere params
	{
		// Rayleigh scattering coefficient (wavelength dependent, RGB for earth-like atmosphere)
		mAtmosphereParams.rayleighScattering = { 0.005802f, 0.013558f, 0.033100f };
		mAtmosphereParams.rayleighDensityExpScale = -0.125f; // Exponential distribution scale height (8km)

		// Mie scattering coefficient (wavelength independent for aerosols)
		mAtmosphereParams.mieScattering = { 0.003996f, 0.003996f, 0.003996f };
		mAtmosphereParams.mieDensityExpScale = -0.833333f; // Exponential distribution scale height (1.2km)
		mAtmosphereParams.mieExtinction = { 0.004440f, 0.004440f, 0.004440f };
		mAtmosphereParams.miePhaseG = 0.8f; // Anisotropy factor (Henyey-Greenstein phase function)
		mAtmosphereParams.mieAbsorption = mAtmosphereParams.mieExtinction - mAtmosphereParams.mieScattering;

		// Planet radii (in km)
		mAtmosphereParams.bottomRadius = 6360.0f; // Earth radius
		mAtmosphereParams.topRadius = 6460.0f; // Atmosphere top (100km above surface)

		// Ozone absorption (creates the blue sky effect by absorbing yellow/red)
		mAtmosphereParams.absorptionExtinction = { 0.000650f, 0.001881f, 0.000085f };

		// Ground albedo (surface reflectance)
		mAtmosphereParams.groundAlbedo = { 0.0f, 0.0f, 0.0f };

		// Ozone absorption density profile (tent/trapezoid function)
		mAtmosphereParams.absorptionDensity0LayerWidth = 25.0f; // Width of absorption layer (km)
		mAtmosphereParams.absorptionDensity0ConstantTerm = -0.6666667f;
		mAtmosphereParams.absorptionDensity0LinearTerm = 0.0666667f;
		mAtmosphereParams.absorptionDensity1ConstantTerm = 2.6666667f;
		mAtmosphereParams.absorptionDensity1LinearTerm = -0.0666667f;
	}
}

void SkyAtmosphere::OnDestroy(RenderContext& context)
{
	context.device->Dispose(context.allocator, mTransmittanceLutTexture);
	context.device->Dispose(context.allocator, mMultiScatterLutTexture);
	context.device->Dispose(context.allocator, mSkyViewLutTexture);
}

void SkyAtmosphere::AddRenderPasses(RenderGraph& graph, RenderGraphBlackboard& blackboard)
{
	const auto& worldRenderingData = blackboard.Get<WorldRenderingData>();
	const auto& sceneData = blackboard.Get<SceneRenderingData>();

	SkyAtmosphereCommonUniforms commonParams = {};
	commonParams.transmittanceLutTexture = mTransmittanceLutTexture.GetResourceView();
	commonParams.multiScatterLutTexture = mMultiScatterLutTexture.GetResourceView();
	commonParams.skyViewLutTexture = mSkyViewLutTexture.GetResourceView();
	commonParams.depthTexture = InvalidResourceIndex;
	commonParams.sunIlluminance = sceneData.sun.illuminance;
	commonParams.sunDirection = sceneData.sun.direction;
	commonParams.rayMarchMinMaxSPP = { 4, 14 };

	auto transmittanceLut = graph.ImportTexture(mTransmittanceLutTexture);
	auto multiScatterLut = graph.ImportTexture(mMultiScatterLutTexture);
	auto skyViewLut = graph.ImportTexture(mSkyViewLutTexture);
	if (mBakeLUTs)
	{
		// Transmittance LUT
		struct SkyAtmosphereTransmittanceLutPassData
		{
			TextureHandle texture;
		};
		graph.AddComputePass<SkyAtmosphereTransmittanceLutPassData>("SkyAtmosphere::TransmittanceLut", [&](RenderGraphBuilder& builder, SkyAtmosphereTransmittanceLutPassData& passData)
		{
			passData.texture = builder.WriteTexture(transmittanceLut);
		},
		[this, commonParams = commonParams](const CommandBuffer* cmd, const SkyAtmosphereTransmittanceLutPassData& passData)
		{
			cmd->BindComputePipeline(mTransmittanceLutPipeline);
			cmd->SetConstantBuffer(mAtmosphereParams, SKY_ATMOSPHERE_PARAMS_BINDING_SLOT);
			cmd->SetConstantBuffer(commonParams, SKY_ATMOSPHERE_COMMON_UNIFORMS_BINDING_SLOT);
			cmd->Dispatch(Math::DivideRoundingUp(SKY_ATMOSPHERE_TRANSMITTANCE_TEXTURE_WIDTH, 16), Math::DivideRoundingUp(SKY_ATMOSPHERE_TRANSMITTANCE_TEXTURE_HEIGHT, 16), 1);
		});

		// Multi scatter LUT
		struct SkyAtmosphereMultiScatterLutPassData
		{
			TextureHandle texture;
			TextureHandle transmittanceLut;
		};
		graph.AddComputePass<SkyAtmosphereMultiScatterLutPassData>("SkyAtmosphere::MultiScatterLut", [&](RenderGraphBuilder& builder, SkyAtmosphereMultiScatterLutPassData& passData)
		{
			passData.texture = builder.WriteTexture(multiScatterLut);
			passData.transmittanceLut = builder.ReadTexture(transmittanceLut);
		},
		[this, commonParams = commonParams](const CommandBuffer* cmd, const SkyAtmosphereMultiScatterLutPassData& passData)
		{
			cmd->BindComputePipeline(mMultiScatterLutPipeline);
			cmd->SetConstantBuffer(mAtmosphereParams, SKY_ATMOSPHERE_PARAMS_BINDING_SLOT);
			cmd->SetConstantBuffer(commonParams, SKY_ATMOSPHERE_COMMON_UNIFORMS_BINDING_SLOT);
			cmd->Dispatch(SKY_ATMOSPHERE_MULTISCATTERING_LUT_RES, SKY_ATMOSPHERE_MULTISCATTERING_LUT_RES, 1);
		});
		mBakeLUTs = false;
	}

	// Sky view LUT - updated every frame
	struct SkyAtmosphereSkyViewLutPassData
	{
		TextureHandle texture;
		TextureHandle transmittanceLut;
		TextureHandle multiScatterLut;
	};
	graph.AddComputePass<SkyAtmosphereSkyViewLutPassData>("SkyAtmosphere::SkyViewLut", [&](RenderGraphBuilder& builder, SkyAtmosphereSkyViewLutPassData& passData)
	{
		passData.texture = builder.WriteTexture(skyViewLut);
		passData.transmittanceLut = builder.ReadTexture(transmittanceLut);
		passData.multiScatterLut = builder.ReadTexture(multiScatterLut);
	},
	[this, sceneData, commonParams = commonParams](const CommandBuffer* cmd, const SkyAtmosphereSkyViewLutPassData& passData)
	{
		cmd->BindComputePipeline(mSkyViewLutPipeline);
		cmd->SetConstantBuffer(mAtmosphereParams, SKY_ATMOSPHERE_PARAMS_BINDING_SLOT);
		cmd->SetConstantBuffer(commonParams, SKY_ATMOSPHERE_COMMON_UNIFORMS_BINDING_SLOT);
		cmd->SetConstantBuffer(sceneData.camera, SKY_ATMOSPHERE_CAMERA_UNIFORMS_BINDING_SLOT);
		cmd->Dispatch(Math::DivideRoundingUp(SKY_ATMOSPHERE_SKY_VIEW_TEXTURE_WIDTH, 16), Math::DivideRoundingUp(SKY_ATMOSPHERE_SKY_VIEW_TEXTURE_HEIGHT, 16), 1);
	});
}