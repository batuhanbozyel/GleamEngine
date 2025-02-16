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

void WorldRenderer::OnCreate(GraphicsDevice* device)
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
	pipelineDesc.sampleCount = Globals::Engine->GetConfiguration().renderer.sampleCount;

    // TODO: create material pipelines
	mShadingPipelines[0] = device->CreateGraphicsPipeline(pipelineDesc);
}

void WorldRenderer::AddRenderPasses(RenderGraph& graph, RenderGraphBlackboard& blackboard)
{
    graph.AddRenderPass<WorldRenderingData>("WorldRenderer::ForwardPass", [&](RenderGraphBuilder& builder, WorldRenderingData& passData)
    {
        const auto& sceneData = blackboard.Get<SceneRenderingData>();
        const auto& backbufferDescriptor = graph.GetDescriptor(sceneData.backbuffer);
        auto sampleCount = Globals::Engine->GetConfiguration().renderer.sampleCount;
        
        RenderTextureDescriptor textureDesc;
        textureDesc.name = "SceneColorRT";
        textureDesc.size = backbufferDescriptor.size;
        textureDesc.sampleCount = sampleCount;
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
        sceneData.sceneProxy->ForEach([this, cmd, passData, sceneData](const Material* material, const TArray<MeshBatch>& batches)
        {
            const auto& materialBuffer = material->GetBuffer();
            const auto& pipeline = mShadingPipelines[material->GetPipelineHash()];

            cmd->BindGraphicsPipeline(pipeline);
			cmd->SetConstantBuffer(sceneData.camera, 1);

            for (const auto& batch : batches)
            {
                const auto& positionBuffer = batch.mesh->GetPositionBuffer();
                const auto& interleavedBuffer = batch.mesh->GetInterleavedBuffer();
				
                MeshPassResources resources;
                resources.positionBuffer = positionBuffer.GetResourceView();
                resources.interleavedBuffer = interleavedBuffer.GetResourceView();
                resources.materialBuffer = materialBuffer.GetResourceView();
				resources.materialID = batch.material->GetID();
				resources.modelMatrix = batch.transform;
				resources.baseVertex = batch.submesh.baseVertex;
                cmd->SetConstantBuffer(resources, 0);
				cmd->DrawIndexed(batch.mesh->GetIndexBuffer(), IndexType::UINT32, batch.submesh.indexCount, 1, batch.submesh.firstIndex);
            }
        });
    });
}
