#include "gpch.h"
#include "ReflectionProbeRenderer.h"

#include "Renderer/CommandBuffer.h"
#include "Renderer/GraphicsDevice.h"

#include "World/World.h"

using namespace Gleam;

void ReflectionProbeRenderer::OnCreate(const RenderContext& context)
{
	ComputePipelineStateDescriptor pipelineState;
	pipelineState.entryPoint = "skyAtmosphereRenderShader";
	mSkyRenderPipeline = context.device->CreateComputePipeline(pipelineState);

	pipelineState.entryPoint = "generateMipsShader";
	mGenerateMipsPipeline = context.device->CreateComputePipeline(pipelineState);

	pipelineState.entryPoint = "diffuseIrradianceConvolutionShader";
	mDiffuseConvolutionPipeline = context.device->CreateComputePipeline(pipelineState);

	pipelineState.entryPoint = "specularPrefilterConvolutionShader";
	mSpecularConvolutionPipeline = context.device->CreateComputePipeline(pipelineState);
}

void ReflectionProbeRenderer::OnDestroy(const RenderContext& context)
{

}

void ReflectionProbeRenderer::AddRenderPasses(RenderGraph& graph, RenderGraphBlackboard& blackboard)
{
	const auto& sceneData = blackboard.Get<SceneRenderingData>();
	const auto& globalProbe = sceneData.world->GetEntityManager().GetSingleton<ReflectionProbe>();

	struct CapturePassData
	{
		TextureHandle probe;
		TextureHandle transmittanceLut;
		TextureHandle multiScatterLut;
	};

	auto& captureData = graph.AddComputePass<CapturePassData>("ReflectionProbe::Capture", [&](RenderGraphBuilder& builder, CapturePassData& passData)
	{
		TextureDescriptor textureDesc;
		textureDesc.name = "GlobalProbe";
		textureDesc.size = (float)globalProbe.resolution;
		textureDesc.usage |= TextureUsage_Storage;
		textureDesc.dimension = TextureDimension::TextureCube;
		textureDesc.format = TextureFormat::R16G16B16A16_SFloat;
		textureDesc.useMipMap = true;
		passData.probe = builder.CreateTexture(textureDesc);
		passData.probe = builder.WriteTexture(passData.probe);

		passData.transmittanceLut = builder.ReadTexture(sceneData.atmosphere.transmittanceLut);
		passData.multiScatterLut = builder.ReadTexture(sceneData.atmosphere.multiScatterLut);
	},
	[this, &sceneData, globalProbe](const CommandBuffer* cmd, const CapturePassData& passData)
	{
		cmd->BindComputePipeline(mSkyRenderPipeline);
		cmd->SetConstantBuffer(sceneData.atmosphere.params, SKY_ATMOSPHERE_PARAMS_BINDING_SLOT);
		cmd->SetConstantBuffer(sceneData.atmosphere.uniforms, SKY_ATMOSPHERE_COMMON_UNIFORMS_BINDING_SLOT);

		for (uint32_t face = 0; face < 6; ++face)
		{
			CameraUniforms cubeFaceCamera = CreateCubeFaceCamera(sceneData.camera.uniforms.position, (uint32_t)globalProbe.resolution, face);

			SkyAtmosphereRenderConstants constants = {};
			constants.targetTexture = passData.probe.GetTexture().GetUnorderedAccessView(0, face);
			constants.depthTexture = InvalidResourceIndex; // No depth test needed
			cmd->SetPushConstant(constants);
			cmd->SetConstantBuffer(cubeFaceCamera, CAMERA_UNIFORMS_BINDING_SLOT);
			cmd->Dispatch(Math::DivideRoundingUp((uint32_t)globalProbe.resolution, 16u), Math::DivideRoundingUp((uint32_t)globalProbe.resolution, 16u), 1u);
		}
	});

	struct MipmapGenerationData
	{
		TextureHandle probe;
	};

	const auto& mipmapData = graph.AddComputePass<MipmapGenerationData>("ReflectionProbe::GenerateMipmaps", [&](RenderGraphBuilder& builder, MipmapGenerationData& passData)
	{
		passData.probe = builder.WriteTexture(captureData.probe);
	},
	[this, &sceneData, globalProbe](const CommandBuffer* cmd, const MipmapGenerationData& passData)
	{
		const auto& probeTexture = passData.probe.GetTexture();

		cmd->BindComputePipeline(mGenerateMipsPipeline);
		for (uint32_t level = 1; level < probeTexture.GetMipMapLevels(); ++level)
		{
			TextureBarrier textureBarrier;
			textureBarrier.resource = probeTexture.GetHandle();
			textureBarrier.srcStage = BarrierStage::ComputeShading;
			textureBarrier.dstStage = BarrierStage::ComputeShading;
			textureBarrier.srcAccess = BarrierAccess::UnorderedAccess;
			textureBarrier.dstAccess = BarrierAccess::UnorderedAccess;
			textureBarrier.oldLayout = BarrierLayout::UnorderedAccess;
			textureBarrier.newLayout = BarrierLayout::UnorderedAccess;

			BarrierGroup barrier;
			barrier.textureBarriers.push_back(textureBarrier);
			cmd->Barrier(barrier);

			uint32_t resolution = (uint32_t)globalProbe.resolution >> level;
			for (uint32_t face = 0; face < 6; ++face)
			{
				GenerateMipsConstants constants = {};
				constants.sourceTexture = probeTexture.GetUnorderedAccessView(level - 1, face);
				constants.targetTexture = probeTexture.GetUnorderedAccessView(level, face);
				constants.resolution = resolution;
				constants.level = level;
				constants.face = face;
				cmd->SetPushConstant(constants);
				cmd->Dispatch(Math::DivideRoundingUp(resolution, 16u), Math::DivideRoundingUp(resolution, 16u), 1u);
			}
		}
	});

	struct DiffuseConvolutionData
	{
		TextureHandle probe;
		TextureHandle targetTexture;
	};

	const auto& diffuseConvolutionData = graph.AddComputePass<DiffuseConvolutionData>("ReflectionProbe::DiffuseConvolution", [&](RenderGraphBuilder& builder, DiffuseConvolutionData& passData)
	{
		TextureDescriptor textureDesc;
		textureDesc.name = "DiffuseIrradianceMap";
		textureDesc.size = Math::DivideRoundingUp((float)globalProbe.resolution, 16.0f);
		textureDesc.usage |= TextureUsage_Storage;
		textureDesc.dimension = TextureDimension::TextureCube;
		textureDesc.format = TextureFormat::R16G16B16A16_SFloat;
		passData.targetTexture = builder.CreateTexture(textureDesc);
		passData.targetTexture = builder.WriteTexture(passData.targetTexture);
		passData.probe = builder.ReadTexture(mipmapData.probe);
	},
	[this, &sceneData, globalProbe](const CommandBuffer* cmd, const DiffuseConvolutionData& passData)
	{
		const auto& targetTexture = passData.targetTexture.GetTexture();
		uint32_t resolution = Math::DivideRoundingUp((uint32_t)globalProbe.resolution, 16u);

		cmd->BindComputePipeline(mDiffuseConvolutionPipeline);
		for (uint32_t face = 0; face < 6; ++face)
		{
			ProbeConvolutionConstants constants = {};
			constants.sourceTexture = passData.probe;
			constants.targetTexture = targetTexture.GetUnorderedAccessView(0, face);
			constants.probeResolution = (uint32_t)globalProbe.resolution;
			constants.resolution = resolution;
			constants.face = face;
			constants.level = 0;

			cmd->SetPushConstant(constants);
			cmd->Dispatch(Math::DivideRoundingUp(resolution, 16u), Math::DivideRoundingUp(resolution, 16u), 1u);
		}
	});

	struct SpecularConvolutionData
	{
		TextureHandle probe;
		TextureHandle targetTexture;
	};

	const auto& specularConvolutionData = graph.AddComputePass<SpecularConvolutionData>("ReflectionProbe::SpecularConvolution", [&](RenderGraphBuilder& builder, SpecularConvolutionData& passData)
	{
		TextureDescriptor textureDesc;
		textureDesc.name = "SpecularRadianceMap";
		textureDesc.size = (float)globalProbe.resolution;
		textureDesc.usage |= TextureUsage_Storage;
		textureDesc.dimension = TextureDimension::TextureCube;
		textureDesc.format = TextureFormat::R16G16B16A16_SFloat;
		textureDesc.useMipMap = true;
		passData.targetTexture = builder.CreateTexture(textureDesc);
		passData.targetTexture = builder.WriteTexture(passData.targetTexture);
		passData.probe = builder.ReadTexture(mipmapData.probe);
	},
	[this, &sceneData, globalProbe](const CommandBuffer* cmd, const SpecularConvolutionData& passData)
	{
		const auto& targetTexture = passData.targetTexture.GetTexture();
		uint32_t maxMipLevel = Math::Min(targetTexture.GetMipMapLevels(), (uint32_t)SPECULAR_RADIANCE_MAX_MIP_COUNT);

		cmd->BindComputePipeline(mSpecularConvolutionPipeline);
		for (uint32_t level = 0; level < maxMipLevel; ++level)
		{
			uint32_t resolution = (uint32_t)globalProbe.resolution >> level;
			for (uint32_t face = 0; face < 6; ++face)
			{
				ProbeConvolutionConstants constants = {};
				constants.sourceTexture = passData.probe;
				constants.targetTexture = targetTexture.GetUnorderedAccessView(level, face);
				constants.probeResolution = (uint32_t)globalProbe.resolution;
				constants.resolution = resolution;
				constants.level = level;
				constants.face = face;

				cmd->SetPushConstant(constants);
				cmd->Dispatch(Math::DivideRoundingUp(resolution, 16u), Math::DivideRoundingUp(resolution, 16u), 1u);
			}
		}
	});

	ReflectionProbePassData passData;
	passData.diffuseReflection = diffuseConvolutionData.targetTexture;
	passData.specularReflection = specularConvolutionData.targetTexture;
	blackboard.Add<ReflectionProbePassData>(passData);
}

CameraUniforms ReflectionProbeRenderer::CreateCubeFaceCamera(const float3& position, uint32_t resolution, uint32_t faceIndex)
{
	static const Float3 directions[6] = {
		Float3( 1,  0,  0), // +X
		Float3(-1,  0,  0), // -X
		Float3( 0,  1,  0), // +Y
		Float3( 0, -1,  0), // -Y
		Float3( 0,  0,  1), // +Z
		Float3( 0,  0, -1)  // -Z
	};
	static const Float3 ups[6] = {
		Float3(0, 1,  0), // +X
		Float3(0, 1,  0), // -X
		Float3(0, 0, -1), // +Y
		Float3(0, 0,  1), // -Y
		Float3(0, 1,  0), // +Z
		Float3(0, 1,  0)  // -Z
	};

	CameraUniforms camera = {};
	camera.resolution = (float)resolution;
	camera.viewMatrix = Float4x4::LookTo(position, directions[faceIndex], ups[faceIndex]);
	camera.projectionMatrix = Float4x4::Perspective(Math::PI_2, 1.0f, 0.1f, 1000.0f); // 90 degree FOV for cubemap
	camera.viewProjectionMatrix = camera.projectionMatrix * camera.viewMatrix;
	camera.invViewMatrix = Math::Inverse(camera.viewMatrix);
	camera.invProjectionMatrix = Math::Inverse(camera.projectionMatrix);
	camera.invViewProjectionMatrix = Math::Inverse(camera.viewProjectionMatrix);
	camera.position = position;
	return camera;
}