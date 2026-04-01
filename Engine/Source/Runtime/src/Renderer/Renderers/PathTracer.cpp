#include "gpch.h"
#include "PathTracer.h"

#include "WorldRenderer.h"
#include "Renderer/CommandBuffer.h"
#include "Renderer/GraphicsDevice.h"

using namespace Gleam;

void PathTracer::OnCreate(RenderContext& context)
{
	mDevice = context.device;
	mAllocator = context.allocator;

	ComputePipelineStateDescriptor pipelineState;
	pipelineState.entryPoint = "pathTraceShader";
	mPathTracingPipeline = context.device->CreateComputePipeline(pipelineState);
}

void PathTracer::OnDestroy(RenderContext& context)
{
	if (mRenderTarget.IsValid())
	{
		mDevice->Dispose(mAllocator, mRenderTarget);
	}
}

void PathTracer::AddRenderPasses(RenderGraph& graph, RenderGraphBlackboard& blackboard)
{
	const auto& sceneData = blackboard.Get<SceneRenderingData>();
	const auto& sceneTargetDescriptor = graph.GetDescriptor(sceneData.sceneTarget);

	if (mRenderTarget.GetDescriptor().size != sceneTargetDescriptor.size)
	{
		mFrameIndex = 0;
		if (mRenderTarget.IsValid())
		{
			mDevice->Dispose(mAllocator, mRenderTarget);
		}

		RenderTextureDescriptor textureDesc;
		textureDesc.size = sceneTargetDescriptor.size;
		textureDesc.name = "SceneColorRT";
		textureDesc.format = TextureFormat::R32G32B32A32_SFloat;
		mRenderTarget = mDevice->CreateTexture(mAllocator, textureDesc);
	}

	if (memcmp(&mState.cameraView, &sceneData.camera.uniforms.viewMatrix, sizeof(float4x4)) != 0 ||
		memcmp(&mState.atmosphereParams, &sceneData.atmosphere.params, sizeof(SkyAtmosphereParameters)) != 0 ||
		memcmp(&mState.atmosphereUniforms, &sceneData.atmosphere.uniforms, sizeof(SkyAtmosphereUniforms)) != 0)
	{
		mState.cameraView = sceneData.camera.uniforms.viewMatrix;
		mState.atmosphereUniforms = sceneData.atmosphere.uniforms;
		mState.atmosphereParams = sceneData.atmosphere.params;
		mFrameIndex = 0;
	}

	auto rtHandle = graph.ImportTexture(mRenderTarget);
	graph.AddComputePass<WorldRenderingData>("PathTracing::Render", [&](RenderGraphBuilder& builder, WorldRenderingData& passData)
	{
		passData.colorTarget = builder.WriteTexture(rtHandle);
		blackboard.Add(passData);
	},
	[this, sceneData](const CommandBuffer* cmd, const WorldRenderingData& passData)
	{
		PathTracerConstants constants = {};
		constants.accelerationStructure = sceneData.accelerationStructure;
		constants.colorTarget = passData.colorTarget;
		constants.frameIndex = mFrameIndex++;
		
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