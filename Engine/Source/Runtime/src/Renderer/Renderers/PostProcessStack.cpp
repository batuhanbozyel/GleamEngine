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
        TextureHandle colorTarget;
        TextureHandle sceneTarget;
    };
    
    graph.AddRenderPass<PostProcessData>("PostProcessStack::Tonemapping", [&](RenderGraphBuilder& builder, PostProcessData& passData)
    {
        auto& renderingData = blackboard.Get<RenderingData>();
        const auto& worldRenderingData = blackboard.Get<WorldRenderingData>();
        passData.colorTarget = builder.UseColorBuffer(renderingData.backbuffer);
        passData.sceneTarget = builder.ReadTexture(worldRenderingData.colorTarget);
        
        renderingData.backbuffer = passData.colorTarget;
    },
    [this](const CommandBuffer* cmd, const PostProcessData& passData)
    {
        PipelineStateDescriptor pipelineDesc;
        cmd->BindGraphicsPipeline(pipelineDesc, mFullscreenTriangleVertexShader, mTonemappingFragmentShader);
        cmd->BindTexture(passData.sceneTarget, 0, ShaderStage_Fragment);
        cmd->Draw(3);
    });
}
