//
//  WorldRenderer.cpp
//  Runtime
//
//  Created by Batuhan Bozyel on 20.10.2022.
//

#include "gpch.h"
#include "WorldRenderer.h"
#include "BRDFRenderer.h"
#include "DepthPrepass.h"
#include "SunShadowRenderer.h"
#include "VisibilityClassification.h"

#include "Core/Engine.h"
#include "Core/Globals.h"

#include "Renderer/Mesh.h"
#include "Renderer/RenderSystem.h"
#include "Renderer/CommandBuffer.h"
#include "Renderer/GraphicsDevice.h"
#include "Renderer/Material/Material.h"
#include "Renderer/Renderers/ReflectionProbeRenderer.h"

#include "World/Systems/RenderSceneProxy.h"

using namespace Gleam;

void WorldRenderer::OnCreate(const RenderContext& context)
{
	mDevice = context.device;
}

void WorldRenderer::OnDestroy(const RenderContext& context)
{

}

void WorldRenderer::AddRenderPasses(RenderGraph& graph, RenderGraphBlackboard& blackboard)
{
	auto renderSystem = Globals::Engine->GetSubsystem<RenderSystem>();
	auto meshShadingPath = renderSystem->GetMeshShadingPath();

	if (meshShadingPath == MeshShadingPath::Visibility)
	{
		AddVisibilityPass(graph, blackboard);
	}
	else
	{
		AddForwardPass(graph, blackboard);
	}
}

void WorldRenderer::AddVisibilityPass(RenderGraph& graph, RenderGraphBlackboard& blackboard)
{
	const auto& sceneData = blackboard.Get<SceneRenderingData>();
	const auto& depthPrepassData = blackboard.Get<DepthPrepassData>();
	const auto& visibilityClassificationData = blackboard.Get<VisibilityClassificationData>();

	struct VisibilityShadingPassData
	{
		WorldRenderingData world;
		TextureHandle visibilityBuffer;
		BufferHandle pixelListBuffer;
		BufferHandle offsetsBuffer;
		BufferHandle countsBuffer;
		BufferHandle dispatchArgsBuffer;
	};

	graph.AddComputePass<VisibilityShadingPassData>("WorldRenderer::VisibilityShading",
	[&](RenderGraphBuilder& builder, VisibilityShadingPassData& passData)
	{
		MakeWorldRenderingData(graph, blackboard, builder, passData.world);

		passData.visibilityBuffer = builder.ReadTexture(depthPrepassData.visibilityBuffer);
		passData.pixelListBuffer = builder.ReadBuffer(visibilityClassificationData.pixelListBuffer);
		passData.offsetsBuffer = builder.ReadBuffer(visibilityClassificationData.offsetsBuffer);
		passData.countsBuffer = builder.ReadBuffer(visibilityClassificationData.countsBuffer);
		passData.dispatchArgsBuffer = builder.ReadBuffer(visibilityClassificationData.dispatchArgsBuffer);
	},
	[this, &sceneData](const CommandBuffer* cmd, const VisibilityShadingPassData& passData)
	{
		sceneData.sceneProxy->ForEach([this, cmd, &passData, &sceneData](const MeshBatch& batch)
		{
			if (batch.numInstances == 0)
			{
				return;
			}

			VisibilityShadingConstants constants = {};
			constants.instanceBuffer = sceneData.sceneProxy->GetGlobalInstanceBuffer().GetResourceView();
			constants.visibilityBuffer = passData.visibilityBuffer;
			constants.pixelListBuffer = passData.pixelListBuffer;
			constants.offsetsBuffer = passData.offsetsBuffer;
			constants.countsBuffer = passData.countsBuffer;
			constants.colorTarget = passData.world.colorTarget;
			constants.brdfTexture = passData.world.brdfLut;
			constants.ggxEssTexture = passData.world.ggxEssLut;
			constants.ggxEAvgTexture = passData.world.ggxEAvgLut;
			constants.diffuseReflectionTexture = passData.world.diffuseReflection;
			constants.specularReflectionTexture = passData.world.specularReflection;
			constants.shadowTexture = passData.world.shadowTexture;
			constants.batchIndex = batch.batchIndex;

			cmd->BindComputePipeline(mVisibilityShadingPipelines[batch.material->GetPipelineHash()]);
			cmd->SetPushConstant(constants);
			cmd->SetConstantBuffer(sceneData.camera.uniforms, CAMERA_UNIFORMS_BINDING_SLOT);
			cmd->SetConstantBuffer(sceneData.atmosphere.params, SKY_ATMOSPHERE_PARAMS_BINDING_SLOT);
			cmd->SetConstantBuffer(sceneData.atmosphere.uniforms, SKY_ATMOSPHERE_COMMON_UNIFORMS_BINDING_SLOT);
			cmd->DispatchIndirect(passData.dispatchArgsBuffer, batch.batchIndex * sizeof(DispatchIndirectArguments));
		});
	});
}

void WorldRenderer::AddForwardPass(RenderGraph& graph, RenderGraphBlackboard& blackboard)
{
	graph.AddRenderPass<WorldRenderingData>("WorldRenderer::ForwardPass", [&](RenderGraphBuilder& builder, WorldRenderingData& passData)
	{
		MakeWorldRenderingData(graph, blackboard, builder, passData);
	},
	[this, &blackboard](const CommandBuffer* cmd, const WorldRenderingData& passData)
	{
		const auto& sceneData = blackboard.Get<SceneRenderingData>();
		sceneData.sceneProxy->ForEach([this, cmd, passData, sceneData](const MeshBatch& batch)
		{
			if (batch.numInstances == 0)
			{
				return;
			}

			const auto globalInstances = sceneData.sceneProxy->GetGlobalInstances();
			const auto globalMeshes = sceneData.sceneProxy->GetGlobalMeshes();

			MeshShadingConstants constants = {};
			constants.instanceBuffer = sceneData.sceneProxy->GetGlobalInstanceBuffer().GetResourceView();
			constants.brdfTexture = passData.brdfLut;
			constants.ggxEssTexture = passData.ggxEssLut;
			constants.ggxEAvgTexture = passData.ggxEAvgLut;
			constants.diffuseReflectionTexture = passData.diffuseReflection;
			constants.specularReflectionTexture = passData.specularReflection;
			constants.shadowTexture = passData.shadowTexture;

			if (mDevice->GetFeatures().meshShaders)
			{
				cmd->BindMeshPipeline(mMeshShadingPipelines[batch.material->GetPipelineHash()]);
				cmd->SetConstantBuffer(sceneData.camera.uniforms, CAMERA_UNIFORMS_BINDING_SLOT);
				cmd->SetConstantBuffer(sceneData.atmosphere.params, SKY_ATMOSPHERE_PARAMS_BINDING_SLOT);
				cmd->SetConstantBuffer(sceneData.atmosphere.uniforms, SKY_ATMOSPHERE_COMMON_UNIFORMS_BINDING_SLOT);

				for (uint32_t instanceID = 0; instanceID < batch.numInstances; ++instanceID)
				{
					constants.instanceID = batch.instanceOffset + instanceID;
					const auto& instance = globalInstances[constants.instanceID];
					cmd->SetPushConstant(constants);
					cmd->DispatchMesh(Math::DivideRoundingUp(instance.meshletCount, MESH_AMPLIFICATION_THREADS), 1, 1);
				}
			}
			else
			{
				cmd->BindGraphicsPipeline(mGraphicsShadingPipelines[batch.material->GetPipelineHash()]);
				cmd->SetConstantBuffer(sceneData.camera.uniforms, CAMERA_UNIFORMS_BINDING_SLOT);
				cmd->SetConstantBuffer(sceneData.atmosphere.params, SKY_ATMOSPHERE_PARAMS_BINDING_SLOT);
				cmd->SetConstantBuffer(sceneData.atmosphere.uniforms, SKY_ATMOSPHERE_COMMON_UNIFORMS_BINDING_SLOT);

				for (uint32_t instanceID = 0; instanceID < batch.numInstances; ++instanceID)
				{
					constants.instanceID = batch.instanceOffset + instanceID;
					const auto& instance = globalInstances[constants.instanceID];
					cmd->SetPushConstant(constants);
					cmd->DrawIndexed(globalMeshes[constants.instanceID].mesh->GetIndexBuffer(), IndexType::UINT32, instance.indexCount, 1, instance.firstIndex);
				}
			}
		});
	});
}

void WorldRenderer::MakeWorldRenderingData(const RenderGraph& graph, RenderGraphBlackboard& blackboard, RenderGraphBuilder& builder, WorldRenderingData& passData)
{
	const auto& sceneData = blackboard.Get<SceneRenderingData>();
	const auto& depthPrepassData = blackboard.Get<DepthPrepassData>();
	const auto& sceneTargetDescriptor = graph.GetDescriptor(sceneData.sceneTarget);

	RenderTextureDescriptor textureDesc;
	textureDesc.name = "SceneColorRT";
	textureDesc.size = sceneTargetDescriptor.size;
	textureDesc.format = TextureFormat::R16G16B16A16_SFloat;
	textureDesc.clearBuffer = true;
	passData.colorTarget = builder.UseColorBuffer(builder.CreateTexture(textureDesc));
	passData.depthTarget = builder.UseDepthBuffer(depthPrepassData.depthTarget, DepthAccess::Read);

	const auto& brdfData = blackboard.Get<BRDFData>();
	passData.brdfLut = builder.ReadTexture(brdfData.brdfLut);
	passData.ggxEssLut = builder.ReadTexture(brdfData.ggxEssLut);
	passData.ggxEAvgLut = builder.ReadTexture(brdfData.ggxEAvgLut);

	if (sceneData.atmosphere.transmittanceLut.IsValid() && sceneData.atmosphere.multiScatterLut.IsValid())
	{
		passData.transmittanceLut = builder.ReadTexture(sceneData.atmosphere.transmittanceLut);
		passData.multiScatterLut = builder.ReadTexture(sceneData.atmosphere.multiScatterLut);
	}

	const auto& reflectionProbeData = blackboard.Get<ReflectionProbePassData>();
	passData.specularReflection = builder.ReadTexture(reflectionProbeData.specularReflection);
	passData.diffuseReflection = builder.ReadTexture(reflectionProbeData.diffuseReflection);

	if (blackboard.Has<SunShadowData>())
	{
		const auto& sunShadowData = blackboard.Get<SunShadowData>();
		passData.shadowTexture = builder.ReadTexture(sunShadowData.shadowMask);
	}
	blackboard.Add(passData);
}

void WorldRenderer::RegisterShadingPipeline(const Material* material)
{
	const auto& materialDesc = material->GetDescriptor();
	auto pipelineHash = material->GetPipelineHash();

	auto it = mGraphicsShadingPipelines.find(pipelineHash);
	if (it == mGraphicsShadingPipelines.end())
	{
		GraphicsPipelineStateDescriptor pipelineDesc = {
			.blendState = {},
			.depthState = DepthState{ .compareFunction = CompareFunction::Equal, .writeEnabled = false },
			.stencilState = StencilState{ .enabled = false },
			.cullingMode = materialDesc.cullingMode,
			.topology = PrimitiveTopology::Triangles,
			.alphaToCoverage = false,
			.wireframe = false,
			.colorFormats = { TextureFormat::R16G16B16A16_SFloat },
			.depthFormat = TextureFormat::D32_SFloat,
			.vertexEntry = "meshVertexShader",
			.fragmentEntry = materialDesc.surfaceShader + "Shading"
		};

		auto pipeline = mDevice->CreateGraphicsPipeline(pipelineDesc);
		mGraphicsShadingPipelines.emplace_hint(it, eastl::piecewise_construct,
											   eastl::forward_as_tuple(pipelineHash),
											   eastl::forward_as_tuple(pipeline));
	}

	if (mDevice->GetFeatures().meshShaders)
	{
		auto meshIt = mMeshShadingPipelines.find(pipelineHash);
		if (meshIt == mMeshShadingPipelines.end())
		{
			MeshPipelineStateDescriptor meshPipelineDesc = {
				.blendState = {},
				.depthState = DepthState{ .compareFunction = CompareFunction::Equal, .writeEnabled = false },
				.stencilState = StencilState{ .enabled = false },
				.cullingMode = materialDesc.cullingMode,
				.alphaToCoverage = false,
				.wireframe = false,
				.colorFormats = { TextureFormat::R16G16B16A16_SFloat },
				.depthFormat = TextureFormat::D32_SFloat,
				.meshEntry = "meshMeshletShader",
				.amplificationEntry = "meshAmplificationShader",
				.fragmentEntry = materialDesc.surfaceShader + "Shading"
			};

			auto meshPipeline = mDevice->CreateMeshPipeline(meshPipelineDesc);
			mMeshShadingPipelines.emplace_hint(meshIt, eastl::piecewise_construct,
											   eastl::forward_as_tuple(pipelineHash),
											   eastl::forward_as_tuple(meshPipeline));
		}

		auto visibilityIt = mVisibilityShadingPipelines.find(pipelineHash);
		if (visibilityIt == mVisibilityShadingPipelines.end())
		{
			ComputePipelineStateDescriptor visibilityPipelineDesc = {
				.entryPoint = materialDesc.surfaceShader + "VisibilityShading"
			};

			auto visibilityPipeline = mDevice->CreateComputePipeline(visibilityPipelineDesc);
			mVisibilityShadingPipelines.emplace_hint(visibilityIt, eastl::piecewise_construct,
													 eastl::forward_as_tuple(pipelineHash),
													 eastl::forward_as_tuple(visibilityPipeline));
		}
	}
}
