#include "gpch.h"
#include "AmbientOcclusionRenderer.h"
#include "DepthPrepass.h"
#include "GBufferResolveRenderer.h"

#include "Renderer/CommandBuffer.h"
#include "Renderer/GraphicsDevice.h"

using namespace Gleam;

namespace {

// CPU port of Intel's XeGTAO::GTAOUpdateConstants for the engine's camera. Depth linearization is
// derived from the near/far planes (viewZ = mul / (add - ndcDepth)); tan(halfFOV) comes from the
// projection diagonal, which is transpose-invariant so it is robust to matrix storage order.
GTAOConstants FillGTAOConstants(const CameraUniforms& camera, uint32_t width, uint32_t height, uint32_t frameIndex)
{
	GTAOConstants consts = {};

	consts.ViewportSize      = int2((int)width, (int)height);
	consts.ViewportPixelSize = float2(1.0f / (float)width, 1.0f / (float)height);

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

	// XeGTAO default tuning (Medium quality preset; single sharp denoise pass).
	consts.EffectRadius             = 0.5f;
	consts.EffectFalloffRange       = 0.615f;
	consts.RadiusMultiplier         = 1.457f;
	consts.FinalValuePower          = 2.2f;
	consts.DenoiseBlurBeta          = 1.2f;   // DenoisePasses > 0
	consts.SampleDistributionPower  = 2.0f;
	consts.ThinOccluderCompensation = 0.0f;
	consts.DepthMIPSamplingOffset   = 3.30f;
	consts.Padding0                 = 0.0f;
	consts.NoiseIndex               = (int)(frameIndex % 64u);

	return consts;
}

} // namespace

void AmbientOcclusionRenderer::OnCreate(const RenderContext& context)
{
	mDevice = context.device;

	ComputePipelineStateDescriptor prefilterDesc;
	prefilterDesc.entryPoint = "gtaoDepthPrefilter";
	mDepthPrefilterPipeline = context.device->CreateComputePipeline(prefilterDesc);

	ComputePipelineStateDescriptor mainPassDesc;
	mainPassDesc.entryPoint = "gtaoUltraMainPass";
	mMainPassPipeline = context.device->CreateComputePipeline(mainPassDesc);

	ComputePipelineStateDescriptor denoiseDesc;
	denoiseDesc.entryPoint = "gtaoDenoise";
	mDenoisePipeline = context.device->CreateComputePipeline(denoiseDesc);
}

void AmbientOcclusionRenderer::AddRenderPasses(RenderGraph& graph, RenderGraphBlackboard& blackboard)
{
	const auto& sceneData        = blackboard.Get<SceneRenderingData>();
	const auto& depthPrepassData = blackboard.Get<DepthPrepassData>();
	const auto& gBufferData      = blackboard.Get<GBufferData>();
	const auto& sceneTargetDescriptor = graph.GetDescriptor(sceneData.sceneTarget);

	const uint32_t width  = (uint32_t)sceneTargetDescriptor.size.width;
	const uint32_t height = (uint32_t)sceneTargetDescriptor.size.height;

	const GTAOConstants gtaoConstants = FillGTAOConstants(sceneData.camera.uniforms, width, height, mFrameIndex);

	// ----------------------------------------------------------------
	// Pass 0 — Depth prefilter: raw NDC depth -> 5-level viewspace depth MIP chain
	// ----------------------------------------------------------------
	struct DepthPrefilterPassData
	{
		TextureHandle sourceDepth;
		TextureHandle workingDepth;
	};

	auto& prefilterData = graph.AddComputePass<DepthPrefilterPassData>("AmbientOcclusion::DepthPrefilter",
	[&](RenderGraphBuilder& builder, DepthPrefilterPassData& passData)
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

	auto& mainData = graph.AddComputePass<MainPassData>("AmbientOcclusion::MainPass",
	[&](RenderGraphBuilder& builder, MainPassData& passData)
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

		cmd->BindComputePipeline(mMainPassPipeline);
		cmd->SetPushConstant(constants);
		cmd->SetConstantBuffer(sceneData.camera.uniforms, CAMERA_UNIFORMS_BINDING_SLOT);
		cmd->Dispatch(Math::DivideRoundingUp(width, 8u), Math::DivideRoundingUp(height, 8u), 1u);
	});

	// ----------------------------------------------------------------
	// Pass 2 — Denoise: edge-aware spatial blur -> final AO
	// ----------------------------------------------------------------
	struct DenoisePassData
	{
		TextureHandle aoTerm;
		TextureHandle edges;
		TextureHandle finalAO;
	};

	auto& denoiseData = graph.AddComputePass<DenoisePassData>("AmbientOcclusion::Denoise",
	[&](RenderGraphBuilder& builder, DenoisePassData& passData)
	{
		RenderTextureDescriptor finalDesc;
		finalDesc.name   = "AmbientOcclusion";
		finalDesc.size   = sceneTargetDescriptor.size;
		finalDesc.format = TextureFormat::R8_UInt;
		passData.finalAO = builder.WriteTexture(builder.CreateTexture(finalDesc));

		passData.aoTerm = builder.ReadTexture(mainData.aoTerm);
		passData.edges  = builder.ReadTexture(mainData.edges);
	},
	[this, gtaoConstants, width, height](const CommandBuffer* cmd, const DenoisePassData& passData)
	{
		GTAODenoiseConstants constants = {};
		constants.gtao           = gtaoConstants;
		constants.sourceAOTerm   = passData.aoTerm;
		constants.sourceEdges    = passData.edges;
		constants.outFinalAOTerm = passData.finalAO;
		constants.finalApply     = 1u;

		cmd->BindComputePipeline(mDenoisePipeline);
		cmd->SetPushConstant(constants);
		cmd->Dispatch(Math::DivideRoundingUp(width, 16u), Math::DivideRoundingUp(height, 8u), 1u);
	});

	AmbientOcclusionData output;
	output.aoTarget = denoiseData.finalAO;
	blackboard.Add(output);

	mFrameIndex++;
}
