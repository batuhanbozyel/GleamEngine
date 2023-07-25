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
#include "Renderer/RendererBindingTable.h"

using namespace Gleam;

void WorldRenderer::OnCreate(RendererContext& context)
{
    mForwardPassVertexShader = context.CreateShader("forwardPassVertexShader", ShaderStage::Vertex);
    mForwardPassFragmentShader = context.CreateShader("forwardPassFragmentShader", ShaderStage::Fragment);
    
    BufferDescriptor descriptor;
    descriptor.memoryType = MemoryType::Dynamic;
    descriptor.usage = BufferUsage::UniformBuffer;
    descriptor.size = sizeof(CameraUniforms);
    mCameraBuffer = CreateScope<Buffer>(descriptor);
}

void WorldRenderer::AddRenderPasses(RenderGraph& graph, RenderGraphBlackboard& blackboard)
{
    const auto& forwardPassData = graph.AddRenderPass<WorldRenderingData>("WorldRenderer::ForwardPass", [&](RenderGraphBuilder& builder, WorldRenderingData& passData)
    {
        const auto& renderingData = blackboard.Get<RenderingData>();
        const auto& backbufferDescriptor = graph.GetDescriptor(renderingData.backbuffer);
        
        TextureDescriptor descriptor;
        descriptor.size = backbufferDescriptor.texture.size;
        descriptor.sampleCount = renderingData.config.sampleCount;
        descriptor.format = TextureFormat::R32G32B32A32_SFloat;
        auto colorTarget = builder.CreateRenderTexture(descriptor);
        
        descriptor.format = TextureFormat::D32_SFloat;
        auto depthTarget = builder.CreateRenderTexture(descriptor);
        
        passData.colorTarget = builder.UseColorBuffer(colorTarget);
        passData.depthTarget = builder.UseDepthBuffer(depthTarget);
        blackboard.Add(passData);
        
        graph.GetDescriptor(colorTarget).clearBuffer = true;
        graph.GetDescriptor(depthTarget).clearBuffer = true;
    },
    [this](const RenderGraphContext& renderGraphContext, const WorldRenderingData& passData)
    {
        for (auto& [material, meshList] : mOpaqueQueue)
        {
            for (const auto& pass : material->GetPasses())
            {
                renderGraphContext.cmd->BindGraphicsPipeline(pass.pipelineState, pass.vertexFunction, pass.fragmentFunction);
                renderGraphContext.cmd->SetVertexBuffer(*mCameraBuffer, 0, RendererBindingTable::CameraBuffer);
                
                for (const auto& element : meshList)
                {
                    const auto& meshBuffer = element.mesh->GetBuffer();
                    renderGraphContext.cmd->SetVertexBuffer(*meshBuffer.GetPositionBuffer(), 0, RendererBindingTable::PositionBuffer);
                    renderGraphContext.cmd->SetVertexBuffer(*meshBuffer.GetInterleavedBuffer(), 0, RendererBindingTable::InterleavedBuffer);
                    
                    ForwardPassUniforms uniforms;
                    uniforms.modelMatrix = element.transform;
                    renderGraphContext.cmd->SetPushConstant(uniforms, ShaderStage_Vertex | ShaderStage_Fragment);
                    for (const auto& descriptor : element.mesh->GetSubmeshDescriptors())
                    {
                        renderGraphContext.cmd->DrawIndexed(meshBuffer.GetIndexBuffer()->GetHandle(), IndexType::UINT32, descriptor.indexCount, 1, descriptor.firstIndex, descriptor.baseVertex, 0);
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
    CameraUniforms cameraData;
    cameraData.viewMatrix = camera.GetViewMatrix();
    cameraData.projectionMatrix = camera.GetProjectionMatrix();
    cameraData.viewProjectionMatrix = cameraData.projectionMatrix * cameraData.viewMatrix;
    mCameraBuffer->SetData(&cameraData, sizeof(CameraUniforms));
}
