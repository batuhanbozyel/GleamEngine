#include "gpch.h"
#include "SkyAtmosphere.h"
#include "Renderer/Shaders/Atmosphere/SkyAtmosphereDefinitions.h"

#include "Renderer/CommandBuffer.h"
#include "Renderer/RenderSurface.h"
#include "Renderer/GraphicsDevice.h"
#include "Renderer/Renderers/WorldRenderer.h"

#include "World/World.h"
#include "World/Components/SkyAtmosphere.h"

using namespace Gleam;

void SkyAtmosphereRenderer::OnCreate(RenderContext& context)
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

	ComputePipelineStateDescriptor pipelineState;
	pipelineState.entryPoint = "skyAtmosphereRenderShader";
	mSkyRenderPipeline = context.device->CreateComputePipeline(pipelineState);
}

void SkyAtmosphereRenderer::OnDestroy(RenderContext& context)
{
	context.device->Dispose(context.allocator, mTransmittanceLutTexture);
	context.device->Dispose(context.allocator, mMultiScatterLutTexture);
}

void SkyAtmosphereRenderer::AddRenderPasses(RenderGraph& graph, RenderGraphBlackboard& blackboard)
{
	const auto& worldRenderingData = blackboard.Get<WorldRenderingData>();
	const auto& sceneData = blackboard.Get<SceneRenderingData>();

	// Convert camera data from meters to kilometers for atmosphere rendering
	CameraUniforms skyCamera = sceneData.camera.uniforms;
	skyCamera.position = sceneData.camera.uniforms.position * 0.001f; // meters to kilometers

	// Update view matrices to account for km scale
	Float4x4 scaleMatrix = Float4x4::Scale(0.001f);
	skyCamera.viewMatrix = scaleMatrix * sceneData.camera.uniforms.viewMatrix;
	skyCamera.viewProjectionMatrix = skyCamera.projectionMatrix * skyCamera.viewMatrix;
	skyCamera.invViewMatrix = sceneData.camera.uniforms.invViewMatrix * Float4x4::Scale(1000.0f);
	skyCamera.invViewProjectionMatrix = Math::Inverse(skyCamera.viewProjectionMatrix);

	bool bakeLUTs = memcmp(&sceneData.atmosphere.params, &mAtmosphereParams, sizeof(SkyAtmosphereParameters)) != 0;
	if (bakeLUTs)
	{
		mAtmosphereParams = sceneData.atmosphere.params;

		// Transmittance LUT
		struct SkyAtmosphereTransmittanceLutPassData
		{
			TextureHandle texture;
		};
		graph.AddComputePass<SkyAtmosphereTransmittanceLutPassData>("SkyAtmosphere::TransmittanceLut", [&](RenderGraphBuilder& builder, SkyAtmosphereTransmittanceLutPassData& passData)
		{
			passData.texture = builder.WriteTexture(sceneData.atmosphere.transmittanceLut);
		},
		[this, sceneData](const CommandBuffer* cmd, const SkyAtmosphereTransmittanceLutPassData& passData)
		{
			cmd->BindComputePipeline(mTransmittanceLutPipeline);
			cmd->SetConstantBuffer(mAtmosphereParams, SKY_ATMOSPHERE_PARAMS_BINDING_SLOT);
			cmd->SetConstantBuffer(sceneData.atmosphere, SKY_ATMOSPHERE_COMMON_UNIFORMS_BINDING_SLOT);
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
			passData.texture = builder.WriteTexture(sceneData.atmosphere.multiScatterLut);
			passData.transmittanceLut = builder.ReadTexture(sceneData.atmosphere.transmittanceLut);
		},
		[this, sceneData](const CommandBuffer* cmd, const SkyAtmosphereMultiScatterLutPassData& passData)
		{
			cmd->BindComputePipeline(mMultiScatterLutPipeline);
			cmd->SetConstantBuffer(mAtmosphereParams, SKY_ATMOSPHERE_PARAMS_BINDING_SLOT);
			cmd->SetConstantBuffer(sceneData.atmosphere, SKY_ATMOSPHERE_COMMON_UNIFORMS_BINDING_SLOT);
			cmd->Dispatch(SKY_ATMOSPHERE_MULTISCATTERING_LUT_RES, SKY_ATMOSPHERE_MULTISCATTERING_LUT_RES, 1);
		});
	}

	// Render sky
	struct SkyAtmospherePassData
	{
		TextureHandle sceneColor;
		TextureHandle sceneDepth;
		TextureHandle transmittanceLut;
		TextureHandle multiScatterLut;
	};
	graph.AddComputePass<SkyAtmospherePassData>("SkyAtmosphere::Render", [&](RenderGraphBuilder& builder, SkyAtmospherePassData& passData)
	{
		auto& worldData = blackboard.Get<WorldRenderingData>();
		passData.sceneColor = builder.WriteTexture(worldData.colorTarget);
		passData.sceneDepth = builder.ReadTexture(worldData.depthTarget);

		passData.transmittanceLut = builder.ReadTexture(sceneData.atmosphere.transmittanceLut);
		passData.multiScatterLut = builder.ReadTexture(sceneData.atmosphere.multiScatterLut);

		worldData.colorTarget = passData.sceneColor;
	},
	[this, sceneData, skyCamera = skyCamera](const CommandBuffer* cmd, const SkyAtmospherePassData& passData)
	{
		SkyAtmosphereRenderConstants constants = {};
		constants.targetTexture = passData.sceneColor.GetTexture().GetResourceView();
		constants.depthTexture = passData.sceneDepth.GetTexture().GetResourceView();

		cmd->BindComputePipeline(mSkyRenderPipeline);
		cmd->SetConstantBuffer(skyCamera, CAMERA_UNIFORMS_BINDING_SLOT);
		cmd->SetConstantBuffer(mAtmosphereParams, SKY_ATMOSPHERE_PARAMS_BINDING_SLOT);
		cmd->SetConstantBuffer(sceneData.atmosphere, SKY_ATMOSPHERE_COMMON_UNIFORMS_BINDING_SLOT);
		cmd->SetPushConstant(constants);
		cmd->Dispatch(Math::DivideRoundingUp((int)skyCamera.resolution.x, 16), Math::DivideRoundingUp((int)skyCamera.resolution.y, 16), 1);
	});
}

SkyAtmosphereParameters SkyAtmosphereRenderer::GetSkyAtmosphereParameters(const Atmosphere& atmosphere) const
{
	SkyAtmosphereParameters params = {};
	params.bottomRadius = atmosphere.planetRadius;
	params.topRadius = atmosphere.planetRadius + atmosphere.atmosphereHeight;
	params.rayleighScattering = float3(atmosphere.rayleighScattering.r * atmosphere.rayleighScatteringLength, atmosphere.rayleighScattering.g * atmosphere.rayleighScatteringLength, atmosphere.rayleighScattering.b * atmosphere.rayleighScatteringLength);
	params.rayleighDensityExpScale = -1.0f / atmosphere.rayleighScaleHeight;
	params.mieScattering = float3(atmosphere.mieScattering.r * atmosphere.mieScatteringLength, atmosphere.mieScattering.g * atmosphere.mieScatteringLength, atmosphere.mieScattering.b * atmosphere.mieScatteringLength);
	params.mieAbsorption = float3(atmosphere.mieAbsorption.r * atmosphere.mieAbsorptionLength, atmosphere.mieAbsorption.g * atmosphere.mieAbsorptionLength, atmosphere.mieAbsorption.b * atmosphere.mieAbsorptionLength);
	params.mieExtinction = params.mieScattering + params.mieAbsorption;
	params.mieDensityExpScale = -1.0f / atmosphere.mieScaleHeight;
	params.miePhaseG = atmosphere.miePhaseG;
	params.absorptionExtinction = float3(atmosphere.absorption.r * atmosphere.absorptionLength, atmosphere.absorption.g * atmosphere.mieAbsorptionLength, atmosphere.absorption.b * atmosphere.absorptionLength);
	params.groundAlbedo = float3(atmosphere.groundAlbedo.r, atmosphere.groundAlbedo.g, atmosphere.groundAlbedo.b);
	params.absorptionDensity0LayerWidth = atmosphere.absorptionDensity0LayerWidth;
	params.absorptionDensity0ConstantTerm = atmosphere.absorptionDensity0ConstantTerm;
	params.absorptionDensity0LinearTerm = atmosphere.absorptionDensity0LinearTerm;
	params.absorptionDensity1ConstantTerm = atmosphere.absorptionDensity1ConstantTerm;
	params.absorptionDensity1LinearTerm = atmosphere.absorptionDensity1LinearTerm;
	return params;
}