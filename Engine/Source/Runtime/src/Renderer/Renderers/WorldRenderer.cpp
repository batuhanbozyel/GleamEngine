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
#include "Renderer/RendererContext.h"

using namespace Gleam;

void WorldRenderer::OnCreate(RendererContext& context)
{
    mForwardPassVertexShader = context.CreateShader("forwardPassVertexShader", ShaderStage::Vertex);
    mForwardPassFragmentShader = context.CreateShader("forwardPassFragmentShader", ShaderStage::Fragment);
}

void WorldRenderer::AddRenderPasses(RenderGraph& graph, RenderGraphBlackboard& blackboard)
{
    struct UpdatePassData
    {
        BufferHandle cameraBuffer;
    };
    
    const auto& updatePass = graph.AddRenderPass<UpdatePassData>("WorldRenderer::UpdatePass", [&](RenderGraphBuilder& builder, UpdatePassData& passData)
    {
        BufferDescriptor descriptor;
        descriptor.usage = BufferUsage::UniformBuffer;
        descriptor.size = sizeof(CameraUniforms);
        passData.cameraBuffer = builder.CreateBuffer(descriptor);
        passData.cameraBuffer = builder.WriteBuffer(passData.cameraBuffer);
    },
    [this](const CommandBuffer* cmd, const UpdatePassData& passData)
    {
        cmd->SetBufferData(passData.cameraBuffer, &mCameraData, sizeof(CameraUniforms));
    });
    
    graph.AddRenderPass<WorldRenderingData>("WorldRenderer::ForwardPass", [&](RenderGraphBuilder& builder, WorldRenderingData& passData)
    {
        const auto& renderingData = blackboard.Get<RenderingData>();
        const auto& backbufferDescriptor = graph.GetDescriptor(renderingData.backbuffer);
        
        RenderTextureDescriptor textureDesc;
        textureDesc.size = backbufferDescriptor.size;
        textureDesc.sampleCount = renderingData.config.sampleCount;
        textureDesc.format = TextureFormat::R32G32B32A32_SFloat;
        textureDesc.clearBuffer = true;
        passData.colorTarget = builder.CreateTexture(textureDesc);
        
        textureDesc.format = TextureFormat::D32_SFloat;
        passData.depthTarget = builder.CreateTexture(textureDesc);
        
        passData.colorTarget = builder.UseColorBuffer(passData.colorTarget);
        passData.depthTarget = builder.UseDepthBuffer(passData.depthTarget);
        passData.cameraBuffer = builder.ReadBuffer(updatePass.cameraBuffer);
        blackboard.Add(passData);
    },
    [this](const CommandBuffer* cmd, const WorldRenderingData& passData)
    {
        for (auto& [material, meshList] : mOpaqueQueue)
        {
            for (const auto& pass : material->GetPasses())
            {
                cmd->BindGraphicsPipeline(pass.pipelineState, pass.vertexFunction, pass.fragmentFunction);
                cmd->BindBuffer(passData.cameraBuffer, 0, 0, ShaderStage_Vertex);
                
                for (const auto& element : meshList)
                {
                    const auto& meshBuffer = element.mesh->GetBuffer();
                    cmd->BindBuffer(meshBuffer.GetPositionBuffer(), 0, 0, ShaderStage_Vertex);
                    cmd->BindBuffer(meshBuffer.GetInterleavedBuffer(), 0, 1, ShaderStage_Vertex);
                    
                    ForwardPassUniforms uniforms;
                    uniforms.modelMatrix = element.transform;
                    cmd->SetPushConstant(uniforms, ShaderStage_Vertex);
                    for (const auto& descriptor : element.mesh->GetSubmeshDescriptors())
                    {
                        cmd->DrawIndexed(meshBuffer.GetIndexBuffer().GetHandle(), IndexType::UINT32, descriptor.indexCount, 1, descriptor.firstIndex, descriptor.baseVertex, 0);
                    }
                }
            }
        }
        mOpaqueQueue.clear();
    });
}

void WorldRenderer::DrawMesh(const MeshRenderer& meshRenderer, const Transform& transform)
{
    RenderQueueElement element;
    element.mesh = meshRenderer.GetMesh();
    element.transform = transform.GetTransform();
    
    if (meshRenderer.GetMaterial()->GetRenderQueue() == RenderQueue::Opaque)
    {
        mOpaqueQueue[meshRenderer.GetMaterial()].emplace_back(element);
    }
    else
    {
        mTransparentQueue[meshRenderer.GetMaterial()].emplace_back(element);
    }
}

void WorldRenderer::UpdateCamera(const Camera& camera)
{
    mCameraData.viewMatrix = camera.GetViewMatrix();
    mCameraData.projectionMatrix = camera.GetProjectionMatrix();
    mCameraData.viewProjectionMatrix = mCameraData.projectionMatrix * mCameraData.viewMatrix;
}
