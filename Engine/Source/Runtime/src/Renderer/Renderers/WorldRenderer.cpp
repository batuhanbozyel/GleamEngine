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
#include "Renderer/Material/Material.h"
#include "Renderer/Material/MaterialInstance.h"
#include "World/Systems/RenderSceneProxy.h"

using namespace Gleam;

void WorldRenderer::OnCreate(RenderContext& context)
{
	GraphicsPipelineStateDescriptor pipelineDesc = {
		PipelineStateDescriptor {
			.blendState = {},
			.depthState = {
			.compareFunction = CompareFunction::Less,
			.writeEnabled = true},
			.stencilState = {},
			.cullingMode = CullMode::Back,
			.topology = PrimitiveTopology::Triangles,
			.alphaToCoverage = false,
			.wireframe = false
		}
	};
	pipelineDesc.colorFormats = { TextureFormat::R16G16B16A16_SFloat };
	pipelineDesc.depthFormat = TextureFormat::D16_UNorm;
	pipelineDesc.vertexEntry = "meshVertexShader";
	pipelineDesc.fragmentEntry = "SurfaceLit";

    // TODO: create material pipelines
	mShadingPipelines[0] = context.device->CreateGraphicsPipeline(pipelineDesc);
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
            const auto& pipeline = mShadingPipelines[0]; // TODO: use batch.material->GetPipelineHash()

			MeshPassResources resources;
			resources.instanceBuffer = batch.instanceBuffer.GetResourceView();
			resources.materialBuffer = materialBuffer.GetResourceView();

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
