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


using namespace Gleam;

void PostProcessStack::OnCreate(const RenderContext& context)
{
	mDevice = context.device;
	mPipelineState.vertexEntry = "fullscreenTriangleVertexShader";
	mPipelineState.fragmentEntry = "tonemappingFragmentShader";
}

void PostProcessStack::AddRenderPasses(RenderGraph& graph, RenderGraphBlackboard& blackboard)
{
	if (not blackboard.Get<SceneRenderingData>().world)
	{
		return;
	}

	auto pipelineState = mPipelineState;
	pipelineState.colorFormats = { graph.GetDescriptor(blackboard.Get<SceneRenderingData>().backbuffer).format };
	const auto pipeline = mDevice->CreateGraphicsPipeline(pipelineState);

    struct PostProcessData
    {
        TextureHandle renderTarget;
        TextureHandle sceneColor;
    };
    
    graph.AddRenderPass<PostProcessData>("PostProcessStack::Tonemapping", [&](RenderGraphBuilder& builder, PostProcessData& passData)
    {
        const auto& sceneData = blackboard.Get<SceneRenderingData>();
        passData.renderTarget = builder.UseColorBuffer(sceneData.backbuffer);
        passData.sceneColor = builder.ReadTexture(sceneData.sceneTarget);
		
		builder.SetViewport(sceneData.viewport);
    },
    [pipeline](const CommandBuffer* cmd, const PostProcessData& passData)
    {
        TonemapUniforms uniforms;
        uniforms.sceneColor = passData.sceneColor;
        
        cmd->BindGraphicsPipeline(pipeline);
        cmd->SetPushConstant(uniforms);
        cmd->Draw(3);
    });
}
