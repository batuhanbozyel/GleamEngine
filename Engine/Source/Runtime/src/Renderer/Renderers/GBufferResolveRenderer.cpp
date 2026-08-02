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
	mAllocator = context.allocator;
}

void GBufferResolveRenderer::OnDestroy(const RenderContext& context)
{
	ReleaseGBufferTargets();
}

void GBufferResolveRenderer::ReleaseGBufferTargets()
{
	for (uint32_t i = 0; i < 2; ++i)
	{
		if (mGeometryNormalTargets[i].IsValid())
		{
			mDevice->Dispose(mAllocator, mGeometryNormalTargets[i], BarrierStage::None);
		}

		if (mShadingNormalTargets[i].IsValid())
		{
			mDevice->Dispose(mAllocator, mShadingNormalTargets[i], BarrierStage::None);
		}

		if (mRoughnessTargets[i].IsValid())
		{
			mDevice->Dispose(mAllocator, mRoughnessTargets[i], BarrierStage::None);
		}
	}
	mGBufferSize = Size::zero;
}

void GBufferResolveRenderer::CreateGBufferTargets(const Size& size)
{
	if (mGBufferSize == size)
	{
		return;
	}
	ReleaseGBufferTargets();

	RenderTextureDescriptor textureDesc;
	textureDesc.dimension = TextureDimension::Texture2D;
	textureDesc.size = size;

	textureDesc.format = TextureFormat::R16G16_SNorm;
	textureDesc.name = "GBufferGeometryNormal 0";
	mGeometryNormalTargets[0] = mDevice->CreateTexture(mAllocator, textureDesc);
	textureDesc.name = "GBufferGeometryNormal 1";
	mGeometryNormalTargets[1] = mDevice->CreateTexture(mAllocator, textureDesc);

	textureDesc.name = "GBufferShadingNormal 0";
	mShadingNormalTargets[0] = mDevice->CreateTexture(mAllocator, textureDesc);
	textureDesc.name = "GBufferShadingNormal 1";
	mShadingNormalTargets[1] = mDevice->CreateTexture(mAllocator, textureDesc);

	textureDesc.format = TextureFormat::R8_UNorm;
	textureDesc.name = "GBufferRoughness 0";
	mRoughnessTargets[0] = mDevice->CreateTexture(mAllocator, textureDesc);
	textureDesc.name = "GBufferRoughness 1";
	mRoughnessTargets[1] = mDevice->CreateTexture(mAllocator, textureDesc);

	mGBufferSize = size;
}

void GBufferResolveRenderer::AddRenderPasses(RenderGraph& graph, RenderGraphBlackboard& blackboard)
{
	const auto& sceneData = blackboard.Get<SceneRenderingData>();
	const auto& depthPrepassData = blackboard.Get<DepthPrepassData>();
	const auto& visibilityClassificationData = blackboard.Get<VisibilityClassificationData>();
	const auto& sceneTargetDescriptor = graph.GetDescriptor(sceneData.sceneTarget);

	CreateGBufferTargets(sceneTargetDescriptor.size);

	const uint32_t prevIndex = mFrameIndex & 1u;
	const uint32_t currIndex = prevIndex ^ 1u;

	TextureHandle geometryNormal = graph.ImportTexture(mGeometryNormalTargets[currIndex]);
	TextureHandle shadingNormal = graph.ImportTexture(mShadingNormalTargets[currIndex]);
	TextureHandle roughness = graph.ImportTexture(mRoughnessTargets[currIndex]);
	TextureHandle previousGeometryNormal = graph.ImportTexture(mGeometryNormalTargets[prevIndex]);
	TextureHandle previousShadingNormal = graph.ImportTexture(mShadingNormalTargets[prevIndex]);
	TextureHandle previousRoughness = graph.ImportTexture(mRoughnessTargets[prevIndex]);

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

		textureDesc.name = "GBufferBarycentricCoords";
		textureDesc.format = TextureFormat::R16G16_SFloat;
		passData.gbuffer.barycentricCoordsTarget = builder.WriteTexture(builder.CreateTexture(textureDesc));

		textureDesc.name = "GBufferBarycentricDerivatives";
		textureDesc.format = TextureFormat::R16G16B16A16_UInt;
		passData.gbuffer.barycentricDerivsTarget = builder.WriteTexture(builder.CreateTexture(textureDesc));

		passData.gbuffer.geometryNormalTarget = builder.WriteTexture(geometryNormal);
		passData.gbuffer.shadingNormalTarget = builder.WriteTexture(shadingNormal);
		passData.gbuffer.roughnessTarget = builder.WriteTexture(roughness);

		passData.visibilityBuffer = builder.ReadTexture(depthPrepassData.visibilityBuffer);
		passData.pixelListBuffer = builder.ReadBuffer(visibilityClassificationData.pixelListBuffer);
		passData.offsetsBuffer = builder.ReadBuffer(visibilityClassificationData.offsetsBuffer);
		passData.countsBuffer = builder.ReadBuffer(visibilityClassificationData.countsBuffer);
		passData.dispatchArgsBuffer = builder.ReadBuffer(visibilityClassificationData.dispatchArgsBuffer);
	},
	[this, &sceneData](const CommandBuffer* cmd, const GBufferResolvePassData& passData)
	{
		GBufferResolveConstants constants = {};
		constants.resolve.instanceBuffer = sceneData.sceneProxy->GetGlobalInstanceBuffer().GetResourceView();
		constants.resolve.visibilityBuffer = passData.visibilityBuffer;
		constants.resolve.pixelListBuffer = passData.pixelListBuffer;
		constants.resolve.offsetsBuffer = passData.offsetsBuffer;
		constants.resolve.countsBuffer = passData.countsBuffer;
		constants.motionVectorTarget = passData.gbuffer.motionVectorTarget;
		constants.geometryNormalTarget = passData.gbuffer.geometryNormalTarget;
		constants.shadingNormalTarget = passData.gbuffer.shadingNormalTarget;
		constants.roughnessTarget = passData.gbuffer.roughnessTarget;
		constants.barycentricCoords = passData.gbuffer.barycentricCoordsTarget;
		constants.barycentricDerivatives = passData.gbuffer.barycentricDerivsTarget;

		sceneData.sceneProxy->ForEach([this, cmd, &passData, &sceneData, &constants](const MeshBatch& batch)
		{
			if (batch.numInstances == 0)
			{
				return;
			}

			constants.resolve.batchIndex = batch.batchIndex;

			cmd->BindComputePipeline(mResolvePipelines[batch.material->GetPipelineHash()]);
			cmd->SetPushConstant(constants);
			cmd->SetConstantBuffer(sceneData.camera.uniforms, CAMERA_UNIFORMS_BINDING_SLOT);
			cmd->DispatchIndirect(passData.dispatchArgsBuffer, batch.batchIndex * sizeof(DispatchIndirectArguments));
		});
	});

	GBufferData gbufferData = resolveData.gbuffer;
	gbufferData.previousGeometryNormalTarget = previousGeometryNormal;
	gbufferData.previousShadingNormalTarget = previousShadingNormal;
	gbufferData.previousRoughnessTarget = previousRoughness;
	blackboard.Add(gbufferData);

	mFrameIndex++;
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
