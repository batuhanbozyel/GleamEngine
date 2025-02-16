#include "InfiniteGridRenderer.h"
#include "ShaderTypes.h"

#include "Renderer/CommandBuffer.h"
#include "Renderer/GraphicsDevice.h"
#include "Renderer/Renderers/WorldRenderer.h"

using namespace GEditor;

void InfiniteGridRenderer::OnCreate(Gleam::GraphicsDevice* device)
{
	Gleam::GraphicsPipelineStateDescriptor pipelineDesc;
	pipelineDesc.depthState.compareFunction = Gleam::CompareFunction::Less;

	pipelineDesc.blendState.enabled = true;
	pipelineDesc.blendState.sourceColorBlendMode = Gleam::BlendMode::SrcAlpha;
	pipelineDesc.blendState.destinationColorBlendMode = Gleam::BlendMode::OneMinusSrcAlpha;
	pipelineDesc.blendState.colorBlendOperation = Gleam::BlendOp::Add;
	pipelineDesc.blendState.alphaBlendOperation = Gleam::BlendOp::Add;
	pipelineDesc.blendState.sourceAlphaBlendMode = Gleam::BlendMode::One;
	pipelineDesc.blendState.destinationAlphaBlendMode = Gleam::BlendMode::OneMinusSrcAlpha;

	pipelineDesc.colorFormats = { Gleam::TextureFormat::R16G16B16A16_SFloat };
	pipelineDesc.depthFormat = Gleam::TextureFormat::D16_UNorm;

	pipelineDesc.vertexEntry = "infiniteGridVertexShader";
	pipelineDesc.fragmentEntry = "infiniteGridFragmentShader";
	pipelineDesc.sampleCount = Gleam::Globals::Engine->GetConfiguration().renderer.sampleCount;
	mPipeline = device->CreateGraphicsPipeline(pipelineDesc);
}

void InfiniteGridRenderer::AddRenderPasses(Gleam::RenderGraph& graph, Gleam::RenderGraphBlackboard& blackboard)
{
    struct PassData
    {
        Gleam::TextureHandle colorTarget;
        Gleam::TextureHandle depthTarget;
    };

    graph.AddRenderPass<PassData>("InfiniteGridPass", [&](Gleam::RenderGraphBuilder& builder, PassData& passData)
    {
        auto& worldData = blackboard.Get<Gleam::WorldRenderingData>();
        passData.colorTarget = builder.UseColorBuffer(worldData.colorTarget);
        passData.depthTarget = builder.UseDepthBuffer(worldData.depthTarget);
        
        worldData.colorTarget = passData.colorTarget;
        worldData.depthTarget = passData.depthTarget;
    },
    [this, blackboard](const Gleam::CommandBuffer* cmd, const PassData& passData)
    {
		const auto& sceneData = blackboard.Get<Gleam::SceneRenderingData>();

		InfiniteGridUniforms uniforms;
		uniforms.majorGridDivision = 10;

		uniforms.majorLineColor = 0xFFB5B5B5;
		uniforms.majorLineWidth = 0.15f;

		uniforms.minorLineColor = 0xFFA5A5A5;
		uniforms.minorLineWidth = 0.04f;
		
        cmd->BindGraphicsPipeline(mPipeline);
		cmd->SetConstantBuffer(sceneData.camera, 0);
		cmd->SetPushConstant(uniforms);
        cmd->Draw(6);
    });
}
