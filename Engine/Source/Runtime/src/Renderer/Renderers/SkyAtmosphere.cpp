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

	// Setup atmosphere params
	{
		// Rayleigh scattering coefficient (wavelength dependent, RGB for earth-like atmosphere)
		mAtmosphereParams.rayleighScattering = { 0.005802f, 0.013558f, 0.0331f };
		mAtmosphereParams.rayleighDensityExpScale = -0.125f; // Exponential distribution scale height (8km)

		// Mie scattering coefficient (wavelength independent for aerosols)
		mAtmosphereParams.mieScattering = { 0.003996f, 0.003996f, 0.003996f };
		mAtmosphereParams.mieDensityExpScale = -0.833333f; // Exponential distribution scale height (1.2km)
		mAtmosphereParams.mieExtinction = { 0.00444f, 0.00444f, 0.00444f };
		mAtmosphereParams.miePhaseG = 0.8f; // Anisotropy factor (Henyey-Greenstein phase function)
		mAtmosphereParams.mieAbsorption = mAtmosphereParams.mieExtinction - mAtmosphereParams.mieScattering;

		// Planet radii (in km)
		mAtmosphereParams.bottomRadius = 6360.0f;
		mAtmosphereParams.topRadius = 6420.0f;

		// Ozone absorption (creates the blue sky effect by absorbing yellow/red)
		mAtmosphereParams.absorptionExtinction = { 0.000650f, 0.001881f, 0.000085f };

		// Ground albedo (surface reflectance)
		mAtmosphereParams.groundAlbedo = { 0.1f, 0.1f, 0.1f };

		// Ozone absorption density profile (tent/trapezoid function)
		mAtmosphereParams.absorptionDensity0LayerWidth = 25.0f;
		mAtmosphereParams.absorptionDensity0ConstantTerm = -0.666667f;
		mAtmosphereParams.absorptionDensity0LinearTerm = 0.066667f;
		mAtmosphereParams.absorptionDensity1ConstantTerm = 2.666667f;
		mAtmosphereParams.absorptionDensity1LinearTerm = -0.066667f;
	}
}

void SkyAtmosphere::OnDestroy(RenderContext& context)
{
	context.device->Dispose(context.allocator, mTransmittanceLutTexture);
}

void SkyAtmosphere::AddRenderPasses(RenderGraph& graph, RenderGraphBlackboard& blackboard)
{
	const auto& worldRenderingData = blackboard.Get<WorldRenderingData>();

	SkyAtmosphereCommonUniforms commonParams = {};
	commonParams.transmittanceLutTexture = mTransmittanceLutTexture.GetResourceView();
	commonParams.multiScatterTexture = InvalidResourceIndex;
	commonParams.skyViewLutTexture = InvalidResourceIndex;
	commonParams.depthTexture = InvalidResourceIndex;
	commonParams.sunIlluminance = { 1.0f, 1.0f, 1.0f };
	commonParams.sunDirection = Math::Normalize(Float3(0.43f, 0.43f, 0.0f));
	commonParams.rayMarchMinMaxSPP = { 4, 14 };

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
		[this, commonParams = commonParams](const CommandBuffer* cmd, const SkyAtmosphereTransmittanceLutPassData& passData)
		{
			cmd->BindComputePipeline(mTransmittanceLutPipeline);
			cmd->SetConstantBuffer(mAtmosphereParams, SKY_ATMOSPHERE_PARAMS_BINDING_SLOT);
			cmd->SetConstantBuffer(commonParams, SKY_ATMOSPHERE_COMMON_UNIFORMS_BINDING_SLOT);
			cmd->Dispatch(SKY_ATMOSPHERE_TRANSMITTANCE_TEXTURE_WIDTH / 16, SKY_ATMOSPHERE_TRANSMITTANCE_TEXTURE_HEIGHT / 16, 1);
		});

		mBakeLUTs = false;
	}
}