#include "gpch.h"
#include "AmbientOcclusionRenderer.h"
#include "DepthPrepass.h"
#include "GBufferResolveRenderer.h"

#include "Renderer/CommandBuffer.h"
#include "Renderer/GraphicsDevice.h"
#include "Renderer/Shaders/AmbientOcclusion/XeGTAO.h"

using namespace Gleam;

static constexpr TStringView GetPipelineEntryPointForQuality(AmbientOcclusionQuality quality)
{
	switch (quality)
	{
		case AmbientOcclusionQuality::Low:    return "gtaoLowMainPass";
		case AmbientOcclusionQuality::Medium: return "gtaoMediumMainPass";
		case AmbientOcclusionQuality::High:   return "gtaoHighMainPass";
		case AmbientOcclusionQuality::Ultra:  return "gtaoUltraMainPass";
		default:
		{
			GLEAM_ASSERT(false, "Invalid ambient occlusion quality level");
			return nullptr;
		}
	}
}

static constexpr uint32_t GetDenoisePassCountForMode(AmbientOcclusionMode mode)
{
	switch (mode)
	{
		case AmbientOcclusionMode::None:   return 0u;
		case AmbientOcclusionMode::Sharp:  return 1u;
		case AmbientOcclusionMode::Medium: return 2u;
		case AmbientOcclusionMode::Soft:   return 3u;
		default:
		{
			GLEAM_ASSERT(false, "Invalid ambient occlusion mode");
			return 1u;
		}
	}
}

static constexpr TStringView GetDenoisePassName(uint32_t passIndex)
{
	switch (passIndex)
	{
		case 0:  return "AmbientOcclusion::Denoise Pass 0";
		case 1:  return "AmbientOcclusion::Denoise Pass 1";
		case 2:  return "AmbientOcclusion::Denoise Pass 2";
		default: return "AmbientOcclusion::Denoise";
	}
}

void AmbientOcclusionRenderer::OnCreate(const RenderContext& context)
{
	mDevice = context.device;
	
	ComputePipelineStateDescriptor prefilterDesc;
	prefilterDesc.entryPoint = "gtaoDepthPrefilter";
	mDepthPrefilterPipeline = context.device->CreateComputePipeline(prefilterDesc);
	
	for (uint32_t i = 0; i <= static_cast<uint32_t>(AmbientOcclusionQuality::Ultra); ++i)
	{
		ComputePipelineStateDescriptor mainPassDesc;
		mainPassDesc.entryPoint = GetPipelineEntryPointForQuality(static_cast<AmbientOcclusionQuality>(i));
		mMainPassPipelines[i] = context.device->CreateComputePipeline(mainPassDesc);
	}

	ComputePipelineStateDescriptor denoiseDesc;
	denoiseDesc.entryPoint = "gtaoDenoise";
	mDenoisePipeline = context.device->CreateComputePipeline(denoiseDesc);
}

void AmbientOcclusionRenderer::AddRenderPasses(RenderGraph& graph, RenderGraphBlackboard& blackboard)
{
	if (mSettings.enable == false)
	{
		return;
	}

	const auto& sceneData        = blackboard.Get<SceneRenderingData>();
	const auto& depthPrepassData = blackboard.Get<DepthPrepassData>();
	const auto& gBufferData      = blackboard.Get<GBufferData>();
	const auto& sceneTargetDescriptor = graph.GetDescriptor(sceneData.sceneTarget);
	
	const uint32_t width  = (uint32_t)sceneTargetDescriptor.size.width;
	const uint32_t height = (uint32_t)sceneTargetDescriptor.size.height;
	
	const GTAOConstants gtaoConstants = SetupGTAOConstants(sceneData.camera.uniforms, mFrameIndex);
	
	// ----------------------------------------------------------------
	// Pass 0 — Depth prefilter: raw NDC depth -> 5-level viewspace depth MIP chain
	// ----------------------------------------------------------------
	struct DepthPrefilterPassData
	{
		TextureHandle sourceDepth;
		TextureHandle workingDepth;
	};
	
	auto& prefilterData = graph.AddComputePass<DepthPrefilterPassData>("AmbientOcclusion::DepthPrefilter", [&](RenderGraphBuilder& builder, DepthPrefilterPassData& passData)
   {
		RenderTextureDescriptor workingDepthDesc;
		workingDepthDesc.name      = "GTAO Working Depth";
		workingDepthDesc.size      = sceneTargetDescriptor.size;
		workingDepthDesc.format    = TextureFormat::R32_SFloat;
		workingDepthDesc.useMipMap = true;
		passData.workingDepth = builder.WriteTexture(builder.CreateTexture(workingDepthDesc));
		passData.sourceDepth  = builder.ReadTexture(depthPrepassData.depthTarget);
	},
	[this, gtaoConstants, width, height](const CommandBuffer* cmd, const DepthPrefilterPassData& passData)
	{
		const auto& workingDepth = passData.workingDepth.GetTexture();
		
		GTAODepthPrefilterConstants constants = {};
		constants.gtao         = gtaoConstants;
		constants.sourceDepth  = passData.sourceDepth;
		constants.outDepthMip0 = workingDepth.GetUnorderedAccessView(0);
		constants.outDepthMip1 = workingDepth.GetUnorderedAccessView(1);
		constants.outDepthMip2 = workingDepth.GetUnorderedAccessView(2);
		constants.outDepthMip3 = workingDepth.GetUnorderedAccessView(3);
		constants.outDepthMip4 = workingDepth.GetUnorderedAccessView(4);
		
		cmd->BindComputePipeline(mDepthPrefilterPipeline);
		cmd->SetPushConstant(constants);
		cmd->Dispatch(Math::DivideRoundingUp(width, 16u), Math::DivideRoundingUp(height, 16u), 1u);
	});
	
	// ----------------------------------------------------------------
	// Pass 1 — Main pass: horizon-based AO term + edges
	// ----------------------------------------------------------------
	struct MainPassData
	{
		TextureHandle workingDepth;
		TextureHandle normalTexture;
		TextureHandle aoTerm;
		TextureHandle edges;
	};
	
	auto& mainData = graph.AddComputePass<MainPassData>("AmbientOcclusion::MainPass", [&](RenderGraphBuilder& builder, MainPassData& passData)
	{
		RenderTextureDescriptor aoTermDesc;
		aoTermDesc.name   = "GTAO Working AO Term";
		aoTermDesc.size   = sceneTargetDescriptor.size;
		aoTermDesc.format = TextureFormat::R8_UInt;
		passData.aoTerm = builder.WriteTexture(builder.CreateTexture(aoTermDesc));
		
		RenderTextureDescriptor edgesDesc;
		edgesDesc.name   = "GTAO Working Edges";
		edgesDesc.size   = sceneTargetDescriptor.size;
		edgesDesc.format = TextureFormat::R8_UNorm;
		passData.edges = builder.WriteTexture(builder.CreateTexture(edgesDesc));
		
		passData.workingDepth  = builder.ReadTexture(prefilterData.workingDepth);
		passData.normalTexture = builder.ReadTexture(gBufferData.shadingNormalTarget);
	},
	[this, gtaoConstants, width, height, &sceneData](const CommandBuffer* cmd, const MainPassData& passData)
	{
		GTAOMainPassConstants constants = {};
		constants.gtao             = gtaoConstants;
		constants.workingDepth     = passData.workingDepth;
		constants.normalTexture    = passData.normalTexture;
		constants.outWorkingAOTerm = passData.aoTerm;
		constants.outWorkingEdges  = passData.edges;
		
		cmd->BindComputePipeline(mMainPassPipelines[(uint32_t)mSettings.quality]);
		cmd->SetPushConstant(constants);
		cmd->SetConstantBuffer(sceneData.camera.uniforms, CAMERA_UNIFORMS_BINDING_SLOT);
		cmd->Dispatch(Math::DivideRoundingUp(width, XE_GTAO_NUMTHREADS_X), Math::DivideRoundingUp(height, XE_GTAO_NUMTHREADS_Y), 1u);
	});
	
	// ----------------------------------------------------------------
	// Pass 2 — Denoise: edge-aware spatial blur -> final AO
	// ----------------------------------------------------------------
	struct DenoisePassData
	{
		TextureHandle sourceAOTerm;
		TextureHandle edges;
		TextureHandle outputAOTerm;
	};

	uint32_t denoisePassCount = Math::Max(1u, GetDenoisePassCountForMode(mSettings.mode));

	TextureHandle finalAOTerm   = mainData.aoTerm;
	TextureHandle denoiseSource = mainData.aoTerm;
	TextureHandle denoiseTarget = TextureHandle();

	for (uint32_t passIndex = 0; passIndex < denoisePassCount; ++passIndex)
	{
		bool lastPass = (passIndex == denoisePassCount - 1u);
		auto& denoiseData = graph.AddComputePass<DenoisePassData>(GetDenoisePassName(passIndex), [&](RenderGraphBuilder& builder, DenoisePassData& passData)
		{
			if (lastPass)
			{
				RenderTextureDescriptor finalDesc;
				finalDesc.name   = "AmbientOcclusion";
				finalDesc.size   = sceneTargetDescriptor.size;
				finalDesc.format = TextureFormat::R8_UInt;
				passData.outputAOTerm = builder.WriteTexture(builder.CreateTexture(finalDesc));
			}
			else
			{
				if (denoiseTarget.IsValid() == false)
				{
					RenderTextureDescriptor pingPongDesc;
					pingPongDesc.name   = "GTAO Working AO Term Pong";
					pingPongDesc.size   = sceneTargetDescriptor.size;
					pingPongDesc.format = TextureFormat::R8_UInt;
					denoiseTarget = builder.CreateTexture(pingPongDesc);
				}
				passData.outputAOTerm = builder.WriteTexture(denoiseTarget);
			}

			passData.sourceAOTerm = builder.ReadTexture(denoiseSource);
			passData.edges        = builder.ReadTexture(mainData.edges);
		},
		[this, gtaoConstants, width, height, lastPass](const CommandBuffer* cmd, const DenoisePassData& passData)
		{
			GTAODenoiseConstants constants = {};
			constants.gtao           = gtaoConstants;
			constants.sourceAOTerm   = passData.sourceAOTerm;
			constants.sourceEdges    = passData.edges;
			constants.outFinalAOTerm = passData.outputAOTerm;
			constants.finalApply     = lastPass ? 1u : 0u;

			cmd->BindComputePipeline(mDenoisePipeline);
			cmd->SetPushConstant(constants);
			cmd->Dispatch(Math::DivideRoundingUp(width, XE_GTAO_NUMTHREADS_X * 2u), Math::DivideRoundingUp(height, XE_GTAO_NUMTHREADS_Y), 1u);
		});

		denoiseTarget = denoiseData.sourceAOTerm;
		denoiseSource = denoiseData.outputAOTerm;
		finalAOTerm   = denoiseData.outputAOTerm;
	}

	AmbientOcclusionData output;
	output.aoTarget = finalAOTerm;
	blackboard.Add(output);
	
	mFrameIndex++;
}

GTAOConstants AmbientOcclusionRenderer::SetupGTAOConstants(const CameraUniforms& camera, uint32_t frameIndex) const
{
	GTAOConstants consts = {};

	consts.ViewportSize      = int2((int)camera.resolution.x, (int)camera.resolution.y);
	consts.ViewportPixelSize = float2(1.0f / camera.resolution.x, 1.0f / camera.resolution.y);

	const float nearZ = camera.nearPlane;
	const float farZ  = camera.farPlane;
	float depthLinearizeMul = (farZ * nearZ) / (farZ - nearZ);
	float depthLinearizeAdd = farZ / (farZ - nearZ);
	if (depthLinearizeMul * depthLinearizeAdd < 0.0f)
	{
		depthLinearizeAdd = -depthLinearizeAdd;
	}
	consts.DepthUnpackConsts = float2(depthLinearizeMul, depthLinearizeAdd);

	const float tanHalfFOVX = 1.0f / camera.projectionMatrix[0][0];
	const float tanHalfFOVY = 1.0f / camera.projectionMatrix[1][1];
	consts.CameraTanHalfFOV = float2(tanHalfFOVX, tanHalfFOVY);

	consts.NDCToViewMul = float2(tanHalfFOVX * 2.0f, tanHalfFOVY * -2.0f);
	consts.NDCToViewAdd = float2(tanHalfFOVX * -1.0f, tanHalfFOVY * 1.0f);
	consts.NDCToViewMul_x_PixelSize = float2(consts.NDCToViewMul.x * consts.ViewportPixelSize.x,
											 consts.NDCToViewMul.y * consts.ViewportPixelSize.y);

	consts.NoiseIndex		= (int)(frameIndex % 64u);
	consts.DenoiseBlurBeta	= (mSettings.mode == AmbientOcclusionMode::None) ? 1e4f : XE_GTAO_DEFAULT_DENOISE_BLUR_BETA;

	return consts;
}

void AmbientOcclusionRenderer::SetSettings(const AmbientOcclusionSettings& settings)
{
	if (memcmp(&mSettings, &settings, sizeof(AmbientOcclusionSettings)) != 0)
	{
		mSettings = settings;
		mFrameIndex = 0;
	}
}
