//
//  CompositeRenderer.cpp
//  Editor
//
//  Created by Batuhan Bozyel on 19.04.2023.
//

#include "gpch.h"
#include "CompositeRenderer.h"
#include "WorldRenderer.h"

#include "Renderer/CommandBuffer.h"
#include "Renderer/RendererContext.h"
#include "Renderer/RendererBindingTable.h"

using namespace Gleam;

void CompositeRenderer::OnCreate(RendererContext* context)
{
    mRendererContext = context;
    mCompositePassVertexShader = context->CreateShader("fullscreenTriangleVertexShader", ShaderStage::Vertex);
    mCompositePassFragmentShader = context->CreateShader("compositePassFragmentShader", ShaderStage::Fragment);
}

void CompositeRenderer::AddRenderPasses(RenderGraph& graph, RenderGraphBlackboard& blackboard)
{
    struct CompositePassData
    {
        RenderTextureHandle swapchainTarget;
        RenderTextureHandle finalColorTarget;
    };
    
    auto swapchainTarget = graph.ImportBackbuffer(mRendererContext->GetSwapchainTarget());
    auto& finalPassData = blackboard.Get<WorldRenderer::FinalPassData>();
    graph.AddRenderPass<CompositePassData>("CompositePass", [&](RenderGraphBuilder& builder, CompositePassData& passData)
    {
        passData.finalColorTarget = builder.ReadRenderTexture(finalPassData.colorTarget);
        passData.swapchainTarget = builder.WriteRenderTexture(swapchainTarget);
    },
    [this](const RenderGraphContext& renderGraphContext, const CompositePassData& passData)
    {
        const auto& finalColorRT = renderGraphContext.registry->GetRenderTexture(passData.finalColorTarget);
        const auto& swapchainRT = renderGraphContext.registry->GetRenderTexture(passData.swapchainTarget);
        
        RenderPassDescriptor renderPassDesc;
        renderPassDesc.size = swapchainRT->GetDescriptor().size;
        
        renderPassDesc.colorAttachments.resize(1);
        renderPassDesc.colorAttachments[0].texture = swapchainRT;
        renderPassDesc.colorAttachments[0].loadAction = AttachmentLoadAction::DontCare;
        renderGraphContext.cmd->BeginRenderPass(renderPassDesc);
        
        PipelineStateDescriptor pipelineDesc;
        renderGraphContext.cmd->BindGraphicsPipeline(pipelineDesc, mCompositePassVertexShader, mCompositePassFragmentShader);
        
        renderGraphContext.cmd->SetFragmentTexture(*finalColorRT, 0);
        renderGraphContext.cmd->Draw(3);
        
        renderGraphContext.cmd->EndRenderPass();
    });
}
