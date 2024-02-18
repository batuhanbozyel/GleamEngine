//
//  PostProcessStack.cpp
//  Runtime
//
//  Created by Batuhan Bozyel on 19.05.2023.
//

#include "gpch.h"
#include "PostProcessStack.h"

#include "Renderer/CommandBuffer.h"
#include "Renderer/GraphicsDevice.h"

#include "WorldRenderer.h"

using namespace Gleam;

void PostProcessStack::OnCreate(GraphicsDevice* device)
{
    mFullscreenTriangleVertexShader = device->CreateShader("fullscreenTriangleVertexShader", ShaderStage::Vertex);
    mTonemappingFragmentShader = device->CreateShader("tonemappingFragmentShader", ShaderStage::Fragment);
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
        auto& sceneData = blackboard.Get<SceneRenderingData>();
        const auto& worldData = blackboard.Get<WorldRenderingData>();
        passData.colorTarget = builder.UseColorBuffer(sceneData.backbuffer);
        passData.sceneTarget = builder.ReadTexture(worldData.colorTarget);
        
        sceneData.backbuffer = passData.colorTarget;
    },
    [this](const CommandBuffer* cmd, const PostProcessData& passData)
    {
        PipelineStateDescriptor pipelineDesc;
        cmd->BindGraphicsPipeline(pipelineDesc, mFullscreenTriangleVertexShader, mTonemappingFragmentShader);
        cmd->BindTexture(passData.sceneTarget, 0, ShaderStage_Fragment);
        cmd->Draw(3);
    });
}
