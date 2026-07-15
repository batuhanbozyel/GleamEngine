#include "gpch.h"
#include "DepthPrepass.h"

#include "Core/Engine.h"
#include "Core/Globals.h"

#include "Renderer/Mesh.h"
#include "Renderer/CommandBuffer.h"
#include "Renderer/GraphicsDevice.h"
#include "Renderer/Material/Material.h"

#include "World/Systems/RenderSceneProxy.h"

using namespace Gleam;

void DepthPrepass::OnCreate(const RenderContext& context)
{
	mDevice = context.device;
}

void DepthPrepass::OnDestroy(const RenderContext& context)
{

}

void DepthPrepass::AddRenderPasses(RenderGraph& graph, RenderGraphBlackboard& blackboard)
{
	const auto& sceneData = blackboard.Get<SceneRenderingData>();
	const auto& sceneTargetDescriptor = graph.GetDescriptor(sceneData.sceneTarget);

	graph.AddRenderPass<DepthPrepassData>("DepthPrepass", [&](RenderGraphBuilder& builder, DepthPrepassData& passData)
	{
		RenderTextureDescriptor textureDesc;
		textureDesc.size = sceneTargetDescriptor.size;
		textureDesc.clearBuffer = true;

		textureDesc.name = "SceneDepthRT";
		textureDesc.format = TextureFormat::D32_SFloat;
		passData.depthTarget = builder.UseDepthBuffer(builder.CreateTexture(textureDesc), DepthAccess::Write);

		textureDesc.name = "VisibilityBuffer";
		textureDesc.format = TextureFormat::R32G32_UInt;
		passData.visibilityBuffer = builder.UseColorBuffer(builder.CreateTexture(textureDesc));

		blackboard.Add(passData);
	},
	[this, &sceneData](const CommandBuffer* cmd, const DepthPrepassData& passData)
	{
		sceneData.sceneProxy->ForEach([this, cmd, passData, sceneData](const MeshBatch& batch)
		{
			if (batch.numInstances == 0)
			{
				return;
			}

			const auto globalInstances = sceneData.sceneProxy->GetGlobalInstances();
			const auto globalMeshes = sceneData.sceneProxy->GetGlobalMeshes();

			cmd->BindMeshPipeline(mPipelines[batch.material->GetPipelineHash()]);
			cmd->SetConstantBuffer(sceneData.camera.uniforms, CAMERA_UNIFORMS_BINDING_SLOT);

			DepthPrepassConstants constants = {};
			constants.instanceBuffer = sceneData.sceneProxy->GetGlobalInstanceBuffer().GetResourceView();
			for (uint32_t instanceID = 0; instanceID < batch.numInstances; ++instanceID)
			{
				constants.instanceID = batch.instanceOffset + instanceID;
				const auto& instance = globalInstances[constants.instanceID];
				cmd->SetPushConstant(constants);
				cmd->DispatchMesh(Math::DivideRoundingUp(instance.meshletCount, MESH_AMPLIFICATION_THREADS), 1, 1);
			}
		});
	});
}

void DepthPrepass::RegisterShadingPipeline(const Material* material)
{
	const auto& materialDesc = material->GetDescriptor();
	auto pipelineHash = material->GetPipelineHash();
	const auto fragmentEntry = materialDesc.alphaMode != AlphaMode::Opaque ? materialDesc.surfaceShader + "DepthPrepass" : "";

	auto it = mPipelines.find(pipelineHash);
	if (it == mPipelines.end())
	{
		MeshPipelineStateDescriptor meshPipelineDesc = {
			.blendState = {},
			.depthState = DepthState{.compareFunction = CompareFunction::Less, .writeEnabled = true },
			.stencilState = StencilState{.enabled = false },
			.cullingMode = materialDesc.cullingMode,
			.alphaToCoverage = false,
			.wireframe = false,
			.colorFormats = { TextureFormat::R32G32_UInt },
			.depthFormat = TextureFormat::D32_SFloat,
			.meshEntry = "depthPrepassMeshletShader",
			.amplificationEntry = "depthPrepassAmplificationShader",
			.fragmentEntry = materialDesc.alphaMode != AlphaMode::Opaque ? materialDesc.surfaceShader + "DepthPrepass" : "opaqueDepthPrepassFragmentShader"
		};
		auto meshPipeline = mDevice->CreateMeshPipeline(meshPipelineDesc);
		mPipelines.emplace_hint(it, eastl::piecewise_construct,
								eastl::forward_as_tuple(pipelineHash),
								eastl::forward_as_tuple(meshPipeline));
	}
}
