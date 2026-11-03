#include "gpch.h"
#include "GBufferResolveRenderer.h"
#include "DepthPrepass.h"
#include "VisibilityClassification.h"

#include "Core/Engine.h"
#include "Core/Globals.h"

#include "Renderer/RenderSystem.h"
#include "Renderer/SceneView.h"
#include "Renderer/CommandBuffer.h"
#include "Renderer/GraphicsDevice.h"
#include "Renderer/Material/Material.h"

#include "World/Systems/RenderSceneProxy.h"

using namespace Gleam;

void GBufferResolveHistory::Release(GraphicsDevice* device, GPUAllocator* allocator)
{
	for (uint32_t i = 0; i < 2; ++i)
	{
		if (geometryNormal[i].IsValid())
		{
			device->Dispose(allocator, geometryNormal[i], BarrierStage::None);
		}

		if (shadingNormal[i].IsValid())
		{
			device->Dispose(allocator, shadingNormal[i], BarrierStage::None);
		}

		if (roughness[i].IsValid())
		{
			device->Dispose(allocator, roughness[i], BarrierStage::None);
		}
	}
}

void GBufferResolveRenderer::OnCreate(const RenderContext& context)
{
	mDevice = context.device;
}

void GBufferResolveRenderer::CreateGBufferTargets(GBufferResolveHistory& history, GPUAllocator* allocator, const Size& size) const
{
	if (not history.geometryNormal[0].IsValid() or history.geometryNormal[0].GetDescriptor().size != size)
	{
		history.Release(mDevice, allocator);

		RenderTextureDescriptor textureDesc;
		textureDesc.dimension = TextureDimension::Texture2D;
		textureDesc.size = size;

		textureDesc.format = TextureFormat::R16G16_SNorm;
		textureDesc.name = "GBufferGeometryNormal 0";
		history.geometryNormal[0] = mDevice->CreateTexture(allocator, textureDesc);
		textureDesc.name = "GBufferGeometryNormal 1";
		history.geometryNormal[1] = mDevice->CreateTexture(allocator, textureDesc);

		textureDesc.name = "GBufferShadingNormal 0";
		history.shadingNormal[0] = mDevice->CreateTexture(allocator, textureDesc);
		textureDesc.name = "GBufferShadingNormal 1";
		history.shadingNormal[1] = mDevice->CreateTexture(allocator, textureDesc);

		textureDesc.format = TextureFormat::R8_UNorm;
		textureDesc.name = "GBufferRoughness 0";
		history.roughness[0] = mDevice->CreateTexture(allocator, textureDesc);
		textureDesc.name = "GBufferRoughness 1";
		history.roughness[1] = mDevice->CreateTexture(allocator, textureDesc);

		history.frameIndex = 0;
		history.firstFrame = true;
	}
}

void GBufferResolveRenderer::AddRenderPasses(RenderGraph& graph, RenderGraphBlackboard& blackboard)
{
	const auto& sceneData = blackboard.Get<SceneRenderingData>();
	if (not sceneData.world)
	{
		return;
	}

	const auto& depthPrepassData = blackboard.Get<DepthPrepassData>();
	const auto& visibilityClassificationData = blackboard.Get<VisibilityClassificationData>();
	const auto& sceneTargetDescriptor = graph.GetDescriptor(sceneData.sceneTarget);

	auto& history = sceneData.view->GetHistory<GBufferResolveHistory>();
	CreateGBufferTargets(history, graph.GetAllocator(), sceneTargetDescriptor.size);

	const uint32_t prevIndex = history.frameIndex & 1u;
	const uint32_t currIndex = prevIndex ^ 1u;

	const ImportResourceParams importParams{ .initialLayout = history.firstFrame ? BarrierLayout::Undefined : BarrierLayout::ShaderResource, .finalLayout = BarrierLayout::ShaderResource };
	TextureHandle geometryNormal = graph.ImportTexture(history.geometryNormal[currIndex], importParams);
	TextureHandle shadingNormal = graph.ImportTexture(history.shadingNormal[currIndex], importParams);
	TextureHandle roughness = graph.ImportTexture(history.roughness[currIndex], importParams);
	TextureHandle previousGeometryNormal = graph.ImportTexture(history.geometryNormal[prevIndex], importParams);
	TextureHandle previousShadingNormal = graph.ImportTexture(history.shadingNormal[prevIndex], importParams);
	TextureHandle previousRoughness = graph.ImportTexture(history.roughness[prevIndex], importParams);

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
		constants.resolve.instanceBuffer = sceneData.sceneProxy->GetInstanceBuffer().GetResourceView();
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

	history.frameIndex++;
	history.firstFrame = false;
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
