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

#if defined(USE_METAL_RENDERER)
#import <Metal/Metal.h>
#elif defined(USE_DIRECTX_RENDERER)
#include "../DirectX/DirectXTransitionManager.h"
#endif

using namespace Gleam;

void WorldRenderer::OnCreate(GraphicsDevice* device)
{
    // TODO: create material pipelines
	mShadingPipelines[0] = {
		.blendState = {},
		.depthState = {
			.compareFunction = CompareFunction::Less,
			.writeEnabled = true},
		.stencilState = {},
		.cullingMode = CullMode::Back,
		.topology = PrimitiveTopology::Triangles,
		.bindPoint = PipelineBindPoint::Graphics,
		.alphaToCoverage = false,
		.wireframe = false
	};
    
    // TODO: create material shaders
    mMeshVertexShader = device->CreateShader("meshVertexShader", ShaderStage::Vertex);
    mMeshShadingFragmentShaders["OpaqueLit"] = device->CreateShader("SurfaceLit", ShaderStage::Fragment);
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
        passData.cameraBuffer = builder.ReadBuffer(sceneData.cameraBuffer);
        blackboard.Add(passData);
    },
    [this, blackboard](const CommandBuffer* cmd, const WorldRenderingData& passData)
    {
        const auto& sceneData = blackboard.Get<SceneRenderingData>();
        sceneData.sceneProxy->ForEach([this, cmd, passData](const Material* material, const TArray<MeshBatch>& batches)
        {
            const auto& materialBuffer = material->GetBuffer();
            const auto& shader = mMeshShadingFragmentShaders[material->GetName()];
            const auto& pipeline = mShadingPipelines[material->GetPipelineHash()];
            cmd->BindGraphicsPipeline(pipeline, mMeshVertexShader, shader);

            for (const auto& batch : batches)
            {
                const auto& positionBuffer = batch.mesh.GetPositionBuffer();
                const auto& interleavedBuffer = batch.mesh.GetInterleavedBuffer();
            #ifdef USE_METAL_RENDERER
                [cmd->GetActiveRenderPass() useResource:positionBuffer.GetHandle() usage : MTLResourceUsageRead stages : MTLRenderStageVertex];
                [cmd->GetActiveRenderPass() useResource:interleavedBuffer.GetHandle() usage : MTLResourceUsageRead stages : MTLRenderStageVertex] ;
            #elif defined(USE_DIRECTX_RENDERER)
                DirectXTransitionManager::TransitionLayout(
                    static_cast<ID3D12GraphicsCommandList7*>(cmd->GetHandle()),
                    static_cast<ID3D12Resource*>(positionBuffer.GetHandle()),
                    D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE
                );

                DirectXTransitionManager::TransitionLayout(
                    static_cast<ID3D12GraphicsCommandList7*>(cmd->GetHandle()),
                    static_cast<ID3D12Resource*>(interleavedBuffer.GetHandle()),
                    D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE
                );
            #endif

                MeshPassResources resources;
                resources.cameraBuffer = passData.cameraBuffer;
                resources.positionBuffer = positionBuffer.GetResourceView();
                resources.interleavedBuffer = interleavedBuffer.GetResourceView();
                resources.materialBuffer = materialBuffer.GetResourceView();
				resources.materialID = batch.material.GetUniqueId();
				resources.modelMatrix = batch.transform;
				resources.baseVertex = batch.submesh.baseVertex;
                cmd->SetConstantBuffer(resources, 0);
				cmd->DrawIndexed(batch.mesh.GetIndexBuffer(), IndexType::UINT32, batch.submesh.indexCount, 1, batch.submesh.firstIndex);
            }
        });
    });
}
