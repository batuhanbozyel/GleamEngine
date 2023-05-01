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
    mVertexShader = context->CreateShader("fullscreenTriangleVertexShader", ShaderStage::Vertex);
    mFragmentShader = context->CreateShader("compositePassFragmentShader", ShaderStage::Fragment);
}

void CompositeRenderer::AddRenderPasses(RenderGraph& graph, RenderGraphBlackboard& blackboard)
{
    struct CompositePassData
    {
        RenderTextureHandle swapchainTarget;
        RenderTextureHandle finalColorTarget;
    };
    
    auto swapchainTarget = graph.ImportBackbuffer(CreateRef<RenderTexture>());
    graph.AddRenderPass<CompositePassData>("CompositePass", [&](RenderGraphBuilder& builder, CompositePassData& passData)
    {
        const auto& renderingData = blackboard.Get<RenderingData>();
        passData.finalColorTarget = builder.ReadRenderTexture(renderingData.colorTarget);
        passData.swapchainTarget = builder.UseColorBuffer({.texture = swapchainTarget, .loadAction = AttachmentLoadAction::Clear});
    },
    [this](const RenderGraphContext& renderGraphContext, const CompositePassData& passData)
    {
        const auto& finalColorRT = renderGraphContext.registry->GetRenderTexture(passData.finalColorTarget);
        
        PipelineStateDescriptor pipelineDesc;
        renderGraphContext.cmd->BindGraphicsPipeline(pipelineDesc, mVertexShader, mFragmentShader);
        
        renderGraphContext.cmd->SetFragmentTexture(*finalColorRT, 0);
        renderGraphContext.cmd->Draw(3);
    });
}
