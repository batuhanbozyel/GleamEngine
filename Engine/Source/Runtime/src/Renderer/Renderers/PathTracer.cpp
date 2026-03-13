#include "gpch.h"
#include "PathTracer.h"

#include "WorldRenderer.h"
#include "Renderer/CommandBuffer.h"
#include "Renderer/GraphicsDevice.h"

using namespace Gleam;

void PathTracer::OnCreate(RenderContext& context)
{
	ComputePipelineStateDescriptor pipelineState;
	pipelineState.entryPoint = "pathTraceShader";
	mPathTracingPipeline = context.device->CreateComputePipeline(pipelineState);
}

void PathTracer::OnDestroy(RenderContext& context)
{
}

void PathTracer::AddRenderPasses(RenderGraph& graph, RenderGraphBlackboard& blackboard)
{
	const auto& sceneData = blackboard.Get<SceneRenderingData>();
	const auto& sceneTargetDescriptor = graph.GetDescriptor(sceneData.sceneTarget);
	
	auto& pathTracerData = graph.AddComputePass<WorldRenderingData>("PathTracing::Render", [&](RenderGraphBuilder& builder, WorldRenderingData& passData)
	{
		RenderTextureDescriptor textureDesc;
		textureDesc.size = sceneTargetDescriptor.size;
		
		textureDesc.name = "SceneColorRT";
		textureDesc.format = TextureFormat::R16G16B16A16_SFloat;
		passData.colorTarget = builder.CreateTexture(textureDesc);
		passData.colorTarget = builder.WriteTexture(passData.colorTarget);
		
		blackboard.Add(passData);
	},
	[this, sceneData](const CommandBuffer* cmd, const WorldRenderingData& passData)
	{
		PathTracerConstants constants = {};
		constants.colorTarget = passData.colorTarget;
		
		cmd->BindComputePipeline(mPathTracingPipeline);
		cmd->SetPushConstant(constants);
		cmd->SetConstantBuffer(sceneData.camera.uniforms, CAMERA_UNIFORMS_BINDING_SLOT);
		cmd->SetConstantBuffer(sceneData.atmosphere.params, SKY_ATMOSPHERE_PARAMS_BINDING_SLOT);
		cmd->SetConstantBuffer(sceneData.atmosphere.uniforms, SKY_ATMOSPHERE_COMMON_UNIFORMS_BINDING_SLOT);
		cmd->Dispatch(Math::DivideRoundingUp((uint32_t)sceneData.camera.uniforms.resolution.x, 16u), Math::DivideRoundingUp((uint32_t)sceneData.camera.uniforms.resolution.y, 16u), 1u);
	});
}

void PathTracer::RegisterShadingPipeline(const MaterialDescriptor& material, uint32_t hash)
{
	// TODO: 
}