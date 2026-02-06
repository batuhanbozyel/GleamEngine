//
//  PostProcessStack.cpp
//  Runtime
//
//  Created by Batuhan Bozyel on 19.05.2023.
//

#include "gpch.h"
#include "PostProcessStack.h"

#include "Renderer/CommandBuffer.h"
#include "Renderer/RenderSurface.h"
#include "Renderer/GraphicsDevice.h"

#include "WorldRenderer.h"

using namespace Gleam;

void PostProcessStack::OnCreate(RenderContext& context)
{
	GraphicsPipelineStateDescriptor pipelineState;
	pipelineState.colorFormats = { context.surface->GetFormat() };
	pipelineState.vertexEntry = "fullscreenTriangleVertexShader";
	pipelineState.fragmentEntry = "tonemappingFragmentShader";
	mPipeline = context.device->CreateGraphicsPipeline(pipelineState);
}

void PostProcessStack::AddRenderPasses(RenderGraph& graph, RenderGraphBlackboard& blackboard)
{
    struct PostProcessData
    {
        TextureHandle renderTarget;
        TextureHandle sceneColor;
    };
    
    graph.AddRenderPass<PostProcessData>("PostProcessStack::Tonemapping", [&](RenderGraphBuilder& builder, PostProcessData& passData)
    {
        const auto& sceneData = blackboard.Get<SceneRenderingData>();
        const auto& worldData = blackboard.Get<WorldRenderingData>();
        passData.renderTarget = builder.UseColorBuffer(sceneData.sceneTarget);
        passData.sceneColor = builder.ReadTexture(worldData.colorTarget);
    },
    [this](const CommandBuffer* cmd, const PostProcessData& passData)
    {
        TonemapUniforms uniforms;
        uniforms.sceneColor = passData.sceneColor;
        
        cmd->BindGraphicsPipeline(mPipeline);
        cmd->SetPushConstant(uniforms);
        cmd->Draw(3);
    });
}
