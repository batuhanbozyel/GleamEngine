#include "gpch.h"
#include "InfiniteGridRenderer.h"
#include "ShaderTypes.h"

#include "Renderer/CommandBuffer.h"
#include "Renderer/GraphicsDevice.h"
#include "Renderer/Renderers/WorldRenderer.h"

using namespace GEditor;

void InfiniteGridRenderer::OnCreate(Gleam::GraphicsDevice* device)
{
    mVertexShader = device->CreateShader("infiniteGridVertexShader", Gleam::ShaderStage::Vertex);
    mFragmentShader = device->CreateShader("infiniteGridFragmentShader", Gleam::ShaderStage::Fragment);
}

void InfiniteGridRenderer::AddRenderPasses(Gleam::RenderGraph& graph, Gleam::RenderGraphBlackboard& blackboard)
{
    struct PassData
    {
        Gleam::TextureHandle colorTarget;
        Gleam::TextureHandle depthTarget;
        Gleam::BufferHandle cameraBuffer;
    };

    graph.AddRenderPass<PassData>("InfiniteGridPass", [&](Gleam::RenderGraphBuilder& builder, PassData& passData)
    {
        auto& worldData = blackboard.Get<Gleam::WorldRenderingData>();
        passData.colorTarget = builder.UseColorBuffer(worldData.colorTarget);
        passData.depthTarget = builder.UseDepthBuffer(worldData.depthTarget);
        passData.cameraBuffer = builder.ReadBuffer(worldData.cameraBuffer);
        
        worldData.colorTarget = passData.colorTarget;
        worldData.depthTarget = passData.depthTarget;
    },
    [this](const Gleam::CommandBuffer* cmd, const PassData& passData)
    {
		InfiniteGridUniforms uniforms;
		uniforms.color = 0xFFFFFFFF;

        Gleam::PipelineStateDescriptor pipelineDesc;
        pipelineDesc.depthState.writeEnabled = true;
        
        cmd->BindGraphicsPipeline(pipelineDesc, mVertexShader, mFragmentShader);
		cmd->BindBuffer(passData.cameraBuffer, 0, 0, Gleam::ShaderStage_Vertex);
		cmd->SetPushConstant(uniforms, Gleam::ShaderStage_Vertex);
        cmd->Draw(6);
    });
}
