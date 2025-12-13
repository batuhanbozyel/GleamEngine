//
//  WorldRenderer.cpp
//  Runtime
//
//  Created by Batuhan Bozyel on 20.10.2022.
//

#include "gpch.h"
#include "WorldRenderer.h"

#include "Core/Engine.h"
#include "Core/Globals.h"

#include "Renderer/Mesh.h"
#include "Renderer/CommandBuffer.h"
#include "Renderer/GraphicsDevice.h"

#include "World/Systems/RenderSceneProxy.h"

using namespace Gleam;

void WorldRenderer::OnCreate(RenderContext& context)
{
	mDevice = context.device;
}

void WorldRenderer::AddRenderPasses(RenderGraph& graph, RenderGraphBlackboard& blackboard)
{
    graph.AddRenderPass<WorldRenderingData>("WorldRenderer::ForwardPass", [&](RenderGraphBuilder& builder, WorldRenderingData& passData)
    {
        const auto& sceneData = blackboard.Get<SceneRenderingData>();
        const auto& backbufferDescriptor = graph.GetDescriptor(sceneData.backbuffer);
        
        RenderTextureDescriptor textureDesc;
        textureDesc.name = "SceneColorRT";
        textureDesc.size = backbufferDescriptor.size;
        textureDesc.format = TextureFormat::R16G16B16A16_SFloat;
        textureDesc.clearBuffer = true;
        passData.colorTarget = builder.CreateTexture(textureDesc);
        
        textureDesc.name = "SceneDepthRT";
        textureDesc.format = TextureFormat::D16_UNorm;
        passData.depthTarget = builder.CreateTexture(textureDesc);
        
        passData.colorTarget = builder.UseColorBuffer(passData.colorTarget);
        passData.depthTarget = builder.UseDepthBuffer(passData.depthTarget);
        blackboard.Add(passData);
    },
    [this, blackboard](const CommandBuffer* cmd, const WorldRenderingData& passData)
    {
        const auto& sceneData = blackboard.Get<SceneRenderingData>();
        sceneData.sceneProxy->ForEach([this, cmd, passData, sceneData](const MeshBatch& batch)
        {
            const auto& materialBuffer = batch.material->GetBuffer();
            const auto& pipeline = mShadingPipelines[batch.material->GetPipelineHash()];

			MeshPassResources resources;
			resources.instanceBuffer = batch.instanceBuffer.GetResourceView();
			resources.materialBuffer = materialBuffer.GetResourceView();
			resources.sun = sceneData.sun;

			cmd->BindGraphicsPipeline(pipeline);
			cmd->SetConstantBuffer(resources, 0);
			cmd->SetConstantBuffer(sceneData.camera, 1);

			for (uint32_t instanceID = 0; instanceID < batch.numInstances; ++instanceID)
			{
				const auto& instance = batch.instances[instanceID];
				cmd->SetConstantBuffer(instance, 2);
				cmd->DrawIndexed(batch.meshes[instanceID]->GetIndexBuffer(), IndexType::UINT32, instance.indexCount, 1, instance.firstIndex);
			}
        });
    });
}

void WorldRenderer::RegisterShadingPipeline(const MaterialDescriptor& material, uint32_t hash)
{
	auto it = mShadingPipelines.find(hash);
	if (it == mShadingPipelines.end())
	{
		GraphicsPipelineStateDescriptor pipelineDesc = {
			.blendState = material.blendState,
			.depthState = material.depthState,
			.stencilState = material.stencilState,
			.cullingMode = material.cullingMode,
			.topology = PrimitiveTopology::Triangles,
			.alphaToCoverage = false,
			.wireframe = false,
			.colorFormats = { TextureFormat::R16G16B16A16_SFloat },
			.depthFormat = TextureFormat::D16_UNorm,
			.vertexEntry = "meshVertexShader",
			.fragmentEntry = material.surfaceShader
		};
		auto pipeline = mDevice->CreateGraphicsPipeline(pipelineDesc);

		mShadingPipelines.emplace_hint(it, eastl::piecewise_construct,
										   eastl::forward_as_tuple(hash),
										   eastl::forward_as_tuple(pipeline));
	}
}
