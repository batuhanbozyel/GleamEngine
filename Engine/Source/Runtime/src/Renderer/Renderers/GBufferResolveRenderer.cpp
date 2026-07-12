#include "gpch.h"
#include "GBufferResolveRenderer.h"
#include "DepthPrepass.h"
#include "VisibilityClassification.h"

#include "Core/Engine.h"
#include "Core/Globals.h"

#include "Renderer/RenderSystem.h"
#include "Renderer/CommandBuffer.h"
#include "Renderer/GraphicsDevice.h"
#include "Renderer/Material/Material.h"

#include "World/Systems/RenderSceneProxy.h"

using namespace Gleam;

void GBufferResolveRenderer::OnCreate(const RenderContext& context)
{
	mDevice = context.device;
}

void GBufferResolveRenderer::OnDestroy(const RenderContext& context)
{

}

void GBufferResolveRenderer::AddRenderPasses(RenderGraph& graph, RenderGraphBlackboard& blackboard)
{
	const auto& sceneData = blackboard.Get<SceneRenderingData>();
	const auto& depthPrepassData = blackboard.Get<DepthPrepassData>();
	const auto& visibilityClassificationData = blackboard.Get<VisibilityClassificationData>();
	const auto& sceneTargetDescriptor = graph.GetDescriptor(sceneData.sceneTarget);

	struct GBufferResolvePassData
	{
		GBufferData gbuffer;
		TextureHandle visibilityBuffer;
		BufferHandle pixelListBuffer;
		BufferHandle offsetsBuffer;
		BufferHandle countsBuffer;
		BufferHandle dispatchArgsBuffer;
	};

	const auto& resolveData = graph.AddComputePass<GBufferResolvePassData>("GBufferResolve",
	[&](RenderGraphBuilder& builder, GBufferResolvePassData& passData)
	{
		RenderTextureDescriptor textureDesc;
		textureDesc.size = sceneTargetDescriptor.size;

		textureDesc.name = "GBufferMotionVector";
		textureDesc.format = TextureFormat::R16G16_SFloat;
		passData.gbuffer.motionVectorTarget = builder.WriteTexture(builder.CreateTexture(textureDesc));

		textureDesc.name = "GBufferGeometryNormal";
		textureDesc.format = TextureFormat::R16G16_SNorm;
		passData.gbuffer.geometryNormalTarget = builder.WriteTexture(builder.CreateTexture(textureDesc));

		textureDesc.name = "GBufferShadingNormal";
		textureDesc.format = TextureFormat::R16G16_SNorm;
		passData.gbuffer.shadingNormalTarget = builder.WriteTexture(builder.CreateTexture(textureDesc));

		textureDesc.name = "GBufferRoughness";
		textureDesc.format = TextureFormat::R8_UNorm;
		passData.gbuffer.roughnessTarget = builder.WriteTexture(builder.CreateTexture(textureDesc));

		passData.visibilityBuffer = builder.ReadTexture(depthPrepassData.visibilityBuffer);
		passData.pixelListBuffer = builder.ReadBuffer(visibilityClassificationData.pixelListBuffer);
		passData.offsetsBuffer = builder.ReadBuffer(visibilityClassificationData.offsetsBuffer);
		passData.countsBuffer = builder.ReadBuffer(visibilityClassificationData.countsBuffer);
		passData.dispatchArgsBuffer = builder.ReadBuffer(visibilityClassificationData.dispatchArgsBuffer);
	},
	[this, &sceneData](const CommandBuffer* cmd, const GBufferResolvePassData& passData)
	{
		GBufferResolveConstants constants = {};
		constants.instanceBuffer = sceneData.sceneProxy->GetGlobalInstanceBuffer().GetResourceView();
		constants.visibilityBuffer = passData.visibilityBuffer;
		constants.pixelListBuffer = passData.pixelListBuffer;
		constants.offsetsBuffer = passData.offsetsBuffer;
		constants.countsBuffer = passData.countsBuffer;
		constants.motionVectorTarget = passData.gbuffer.motionVectorTarget;
		constants.geometryNormalTarget = passData.gbuffer.geometryNormalTarget;
		constants.shadingNormalTarget = passData.gbuffer.shadingNormalTarget;
		constants.roughnessTarget = passData.gbuffer.roughnessTarget;

		sceneData.sceneProxy->ForEach([this, cmd, &passData, &sceneData, &constants](const MeshBatch& batch)
		{
			if (batch.numInstances == 0)
			{
				return;
			}

			constants.batchIndex = batch.batchIndex;

			cmd->BindComputePipeline(mResolvePipelines[batch.material->GetPipelineHash()]);
			cmd->SetPushConstant(constants);
			cmd->SetConstantBuffer(sceneData.camera.uniforms, CAMERA_UNIFORMS_BINDING_SLOT);
			cmd->DispatchIndirect(passData.dispatchArgsBuffer, batch.batchIndex * sizeof(DispatchIndirectArguments));
		});
	});

	GBufferData gbufferData = resolveData.gbuffer;
	blackboard.Add(gbufferData);
}

void GBufferResolveRenderer::RegisterShadingPipeline(const Material* material)
{
	if (mDevice->GetFeatures().meshShaders == false)
	{
		return;
	}

	const auto& materialDesc = material->GetDescriptor();
	auto pipelineHash = material->GetPipelineHash();

	auto it = mResolvePipelines.find(pipelineHash);
	if (it == mResolvePipelines.end())
	{
		ComputePipelineStateDescriptor pipelineDesc = {
			.entryPoint = materialDesc.surfaceShader + "GBufferResolve"
		};

		auto pipeline = mDevice->CreateComputePipeline(pipelineDesc);
		mResolvePipelines.emplace_hint(it, eastl::piecewise_construct,
									   eastl::forward_as_tuple(pipelineHash),
									   eastl::forward_as_tuple(pipeline));
	}
}
