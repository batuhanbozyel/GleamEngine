//
//  PostProcessStack.cpp
//  Runtime
//
//  Created by Batuhan Bozyel on 19.05.2023.
//

#include "gpch.h"
#include "PostProcessStack.h"

#include "Renderer/CommandBuffer.h"
#include "Renderer/RendererContext.h"
#include "Renderer/RendererBindingTable.h"

#include "WorldRenderer.h"

using namespace Gleam;

void PostProcessStack::OnCreate(RendererContext& context)
{
    mFullscreenTriangleVertexShader = context.CreateShader("fullscreenTriangleVertexShader", ShaderStage::Vertex);
    mTonemappingFragmentShader = context.CreateShader("tonemappingFragmentShader", ShaderStage::Fragment);
}

void PostProcessStack::AddRenderPasses(RenderGraph& graph, RenderGraphBlackboard& blackboard)
{
    struct PostProcessData
    {
        RenderTextureHandle colorTarget;
        RenderTextureHandle sceneTarget;
    };
    
    graph.AddRenderPass<PostProcessData>("PostProcessStack::Tonemapping", [&](RenderGraphBuilder& builder, PostProcessData& passData)
    {
        auto& renderingData = blackboard.Get<RenderingData>();
        const auto& worldRenderingData = blackboard.Get<WorldRenderingData>();
        passData.colorTarget = builder.UseColorBuffer(renderingData.backbuffer);
        passData.sceneTarget = builder.ReadRenderTexture(worldRenderingData.colorTarget);
        
        renderingData.backbuffer = passData.colorTarget;
    },
    [this](const RenderGraphContext& renderGraphContext, const PostProcessData& passData)
    {
        const auto& sceneRT = renderGraphContext.registry->GetRenderTexture(passData.sceneTarget);
        
        PipelineStateDescriptor pipelineDesc;
        renderGraphContext.cmd->BindGraphicsPipeline(pipelineDesc, mFullscreenTriangleVertexShader, mTonemappingFragmentShader);
        renderGraphContext.cmd->SetFragmentTexture(*sceneRT, 0);
        renderGraphContext.cmd->Draw(3);
    });
}
