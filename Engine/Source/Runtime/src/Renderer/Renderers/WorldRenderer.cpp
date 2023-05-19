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
    
    mFullscreenTriangleVertexShader = context->CreateShader("fullscreenTriangleVertexShader", ShaderStage::Vertex);
    mPostprocessFragmentShader = context->CreateShader("postprocessFragmentShader", ShaderStage::Fragment);
    
    BufferDescriptor descriptor;
    descriptor.memoryType = MemoryType::Dynamic;
    descriptor.usage = BufferUsage::UniformBuffer;
    descriptor.size = sizeof(CameraUniforms);
    mCameraBuffer = CreateScope<Buffer>(descriptor);
}

void WorldRenderer::AddRenderPasses(RenderGraph& graph, RenderGraphBlackboard& blackboard)
{
    TextureDescriptor descriptor;
    descriptor.size = mRendererContext->GetDrawableSize();
    descriptor.sampleCount = mRendererContext->GetConfiguration().sampleCount;
    
    descriptor.format = TextureFormat::R32G32B32A32_SFloat;
    auto colorTarget = CreateRef<RenderTexture>(descriptor);
    
    descriptor.format = TextureFormat::D32_SFloat;
    auto depthTarget = CreateRef<RenderTexture>(descriptor);
    
    WorldRenderingData renderingData;
    renderingData.colorTarget = graph.ImportBackbuffer(colorTarget);
    renderingData.depthTarget = graph.ImportBackbuffer(depthTarget);
    blackboard.Add(renderingData);
    
    struct ForwardPassData
    {
        RenderTextureHandle colorTarget;
        RenderTextureHandle depthTarget;
    };
    
    const auto& forwardPassData = graph.AddRenderPass<ForwardPassData>("WorldRenderer::ForwardPass", [&](RenderGraphBuilder& builder, ForwardPassData& passData)
    {
        passData.colorTarget = builder.UseColorBuffer({.texture = renderingData.colorTarget, .loadAction = AttachmentLoadAction::Clear, .storeAction = mRendererContext->GetConfiguration().sampleCount > 1 ? AttachmentStoreAction::Resolve : AttachmentStoreAction::Store});
        passData.depthTarget = builder.UseDepthBuffer({.texture = renderingData.depthTarget, .loadAction = AttachmentLoadAction::Clear, .storeAction = mRendererContext->GetConfiguration().sampleCount > 1 ? AttachmentStoreAction::Resolve : AttachmentStoreAction::Store});
    },
    [this](const RenderGraphContext& renderGraphContext, const ForwardPassData& passData)
    {
        PipelineStateDescriptor pipelineDesc;
        pipelineDesc.cullingMode = CullMode::Back;
        pipelineDesc.depthState.writeEnabled = true;
        renderGraphContext.cmd->BindGraphicsPipeline(pipelineDesc, mForwardPassVertexShader, mForwardPassFragmentShader);
        
        renderGraphContext.cmd->SetVertexBuffer(*mCameraBuffer, 0, RendererBindingTable::CameraBuffer);
        
        for (const auto& element : mOpaqueQueue)
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
        
        mOpaqueQueue.clear();
    });
    
    struct PostProcessData
    {
        RenderTextureHandle colorTarget;
        RenderTextureHandle sceneTarget;
    };
    
    graph.AddRenderPass<PostProcessData>("WorldRenderer::PostProcess", [&](RenderGraphBuilder& builder, PostProcessData& passData)
    {
        const auto& renderingData = blackboard.Get<RenderingData>();
        passData.colorTarget = builder.UseColorBuffer({.texture = renderingData.swapchainTarget, .loadAction = AttachmentLoadAction::Clear, .storeAction = mRendererContext->GetConfiguration().sampleCount > 1 ? AttachmentStoreAction::Resolve : AttachmentStoreAction::Store});
        passData.sceneTarget = builder.ReadRenderTexture(forwardPassData.colorTarget);
    },
    [this](const RenderGraphContext& renderGraphContext, const PostProcessData& passData)
    {
        const auto& sceneRT = renderGraphContext.registry->GetRenderTexture(passData.sceneTarget);
        
        PipelineStateDescriptor pipelineDesc;
        renderGraphContext.cmd->BindGraphicsPipeline(pipelineDesc, mFullscreenTriangleVertexShader, mPostprocessFragmentShader);
        renderGraphContext.cmd->SetFragmentTexture(*sceneRT, 0);
        renderGraphContext.cmd->Draw(3);
    });
}

void WorldRenderer::DrawMesh(const MeshRenderer& meshRenderer, const Transform& transform)
{
    RenderQueueElement element;
    element.mesh = meshRenderer.GetMesh();
    element.transform = transform.GetTransform();
    mOpaqueQueue.emplace_back(element);
}

void WorldRenderer::UpdateCamera(const Camera& camera)
{
    CameraUniforms cameraData;
    cameraData.viewMatrix = camera.GetViewMatrix();
    cameraData.projectionMatrix = camera.GetProjectionMatrix();
    cameraData.viewProjectionMatrix = cameraData.projectionMatrix * cameraData.viewMatrix;
    mCameraBuffer->SetData(&cameraData, sizeof(CameraUniforms));
}
