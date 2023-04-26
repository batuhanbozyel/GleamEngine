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

void WorldRenderer::OnCreate(RendererContext* context)
{
    mRendererContext = context;
    
    mForwardPassVertexShader = context->CreateShader("forwardPassVertexShader", ShaderStage::Vertex);
    mForwardPassFragmentShader = context->CreateShader("forwardPassFragmentShader", ShaderStage::Fragment);
    
    BufferDescriptor descriptor;
    descriptor.memoryType = MemoryType::Dynamic;
    descriptor.usage = BufferUsage::UniformBuffer;
    descriptor.size = sizeof(CameraUniforms);
    mCameraBuffer = CreateScope<Buffer>(descriptor);
}

void WorldRenderer::AddRenderPasses(RenderGraph& graph, RenderGraphBlackboard& blackboard)
{
    const auto& finalPassData = graph.AddRenderPass<FinalPassData>("WorldRenderer::ForwardPass", [&](RenderGraphBuilder& builder, FinalPassData& passData)
    {
        TextureDescriptor attachmentDesc;
        attachmentDesc.size = mRendererContext->GetDrawableSize();
        attachmentDesc.sampleCount = mRendererContext->GetConfiguration().sampleCount;
        
        attachmentDesc.format = TextureFormat::R32G32B32A32_SFloat;
        passData.colorTarget = builder.CreateRenderTexture(attachmentDesc);
        passData.colorTarget = builder.WriteRenderTexture(passData.colorTarget);
        
        attachmentDesc.format = TextureFormat::D32_SFloat_S8_UInt;
        passData.depthTarget = builder.CreateRenderTexture(attachmentDesc);
        passData.depthTarget = builder.WriteRenderTexture(passData.depthTarget);
    },
    [this](const RenderGraphContext& renderGraphContext, const FinalPassData& passData)
    {
        const auto& colorAttachment = renderGraphContext.registry->GetRenderTexture(passData.colorTarget);
        const auto& depthAttachment = renderGraphContext.registry->GetRenderTexture(passData.depthTarget);
        
        RenderPassDescriptor renderPassDesc;
        renderPassDesc.size = mRendererContext->GetDrawableSize();
        renderPassDesc.samples = mRendererContext->GetConfiguration().sampleCount;
        
        renderPassDesc.colorAttachments.resize(1);
        renderPassDesc.colorAttachments[0].texture = colorAttachment;
        renderPassDesc.colorAttachments[0].loadAction = AttachmentLoadAction::Clear;
        if (renderPassDesc.samples > 1) { renderPassDesc.colorAttachments[0].storeAction = AttachmentStoreAction::Resolve; } ;
        
        renderPassDesc.depthAttachment.texture = depthAttachment;
        renderPassDesc.depthAttachment.loadAction = AttachmentLoadAction::Clear;
        if (renderPassDesc.samples > 1) { renderPassDesc.depthAttachment.storeAction = AttachmentStoreAction::Resolve; } ;
        
        renderGraphContext.cmd->BeginRenderPass(renderPassDesc, "WorldRenderer::ForwardPass");
        
        PipelineStateDescriptor pipelineDesc;
        pipelineDesc.cullingMode = CullMode::Back;
        pipelineDesc.depthState.writeEnabled = true;
        pipelineDesc.stencilState.enabled = true;
        renderGraphContext.cmd->BindGraphicsPipeline(pipelineDesc, mForwardPassVertexShader, mForwardPassFragmentShader);
        
        renderGraphContext.cmd->SetVertexBuffer(*mCameraBuffer, 0, RendererBindingTable::CameraBuffer);
        
        
        renderGraphContext.cmd->EndRenderPass();
    });
    blackboard.Add(finalPassData);
}

void WorldRenderer::DrawMesh(const MeshRenderer& meshRenderer, const Matrix4& transform)
{
    
}

void WorldRenderer::UpdateCamera(const Camera& camera)
{
    CameraUniforms cameraData;
    cameraData.viewMatrix = camera.GetViewMatrix();
    cameraData.projectionMatrix = camera.GetProjectionMatrix();
    cameraData.viewProjectionMatrix = cameraData.projectionMatrix * cameraData.viewMatrix;
    mCameraBuffer->SetData(&cameraData, sizeof(CameraUniforms));
}
