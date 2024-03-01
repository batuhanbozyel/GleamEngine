//
//  WorldRenderer.cpp
//  Runtime
//
//  Created by Batuhan Bozyel on 20.10.2022.
//

#include "gpch.h"
#include "WorldRenderer.h"

#include "Renderer/Mesh.h"
#include "Renderer/CommandBuffer.h"
#include "Renderer/GraphicsDevice.h"
#include "Renderer/Material/Material.h"
#include "Renderer/Material/MaterialInstance.h"

using namespace Gleam;

void WorldRenderer::OnCreate(GraphicsDevice* device)
{
    mForwardPassVertexShader = device->CreateShader("forwardPassVertexShader", ShaderStage::Vertex);
    mForwardPassFragmentShader = device->CreateShader("forwardPassFragmentShader", ShaderStage::Fragment);
}

void WorldRenderer::AddRenderPasses(RenderGraph& graph, RenderGraphBlackboard& blackboard)
{
    graph.AddRenderPass<WorldRenderingData>("WorldRenderer::ForwardPass", [&](RenderGraphBuilder& builder, WorldRenderingData& passData)
    {
        const auto& sceneData = blackboard.Get<SceneRenderingData>();
        const auto& backbufferDescriptor = graph.GetDescriptor(sceneData.backbuffer);
        
        RenderTextureDescriptor textureDesc;
        textureDesc.size = backbufferDescriptor.size;
        textureDesc.sampleCount = sceneData.config.sampleCount;
        textureDesc.format = TextureFormat::R16G16B16A16_SFloat;
        textureDesc.clearBuffer = true;
        passData.colorTarget = builder.CreateTexture(textureDesc);
        
        textureDesc.format = TextureFormat::D16_UNorm;
        passData.depthTarget = builder.CreateTexture(textureDesc);
        
        passData.colorTarget = builder.UseColorBuffer(passData.colorTarget);
        passData.depthTarget = builder.UseDepthBuffer(passData.depthTarget);
        passData.cameraBuffer = builder.ReadBuffer(sceneData.cameraBuffer);
        blackboard.Add(passData);
    },
    [this](const CommandBuffer* cmd, const WorldRenderingData& passData)
    {
        for (auto& [material, meshList] : mOpaqueQueue)
        {
            for (const auto& pass : material->GetPasses())
            {
                cmd->BindGraphicsPipeline(pass.pipelineState, pass.vertexFunction, pass.fragmentFunction);
                
                for (const auto& element : meshList)
                {
                    const auto& meshBuffer = element.mesh->GetBuffer();
                    
                    ForwardPassUniforms uniforms;
                    uniforms.modelMatrix = element.transform;
                    uniforms.cameraBuffer = passData.cameraBuffer;
                    uniforms.positionBuffer = meshBuffer.GetPositionBuffer();
                    uniforms.interleavedBuffer = meshBuffer.GetInterleavedBuffer();
                    cmd->SetPushConstant(uniforms);
					
                    for (const auto& descriptor : element.mesh->GetSubmeshDescriptors())
                    {
                        cmd->DrawIndexed(meshBuffer.GetIndexBuffer(), IndexType::UINT32, descriptor.indexCount, 1, descriptor.firstIndex, descriptor.baseVertex, 0);
                    }
                }
            }
        }
        mOpaqueQueue.clear();
    });
}

void WorldRenderer::DrawMesh(const MeshRenderer& meshRenderer, const Transform& transform)
{
    GLEAM_ASSERT(meshRenderer.GetMesh()->GetSubmeshCount() > 0);
    
    const auto& material = meshRenderer.GetMaterial(0);
    const auto& baseMaterial = std::static_pointer_cast<Material>(material->GetBaseMaterial());
    if (baseMaterial->GetRenderQueue() == RenderQueue::Opaque)
    {
        mOpaqueQueue[baseMaterial].push_back({ meshRenderer.GetMesh().get(), meshRenderer.GetMaterial(0).get(), transform.GetTransform() });
    }
    else
    {
        mTransparentQueue[baseMaterial].push_back({ meshRenderer.GetMesh().get(), meshRenderer.GetMaterial(0).get(), transform.GetTransform() });
    }
}
