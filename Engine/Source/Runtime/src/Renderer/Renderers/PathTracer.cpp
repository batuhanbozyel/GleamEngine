#include "gpch.h"
#include "PathTracer.h"

#include "WorldRenderer.h"
#include "Renderer/RenderSystem.h"
#include "Renderer/CommandBuffer.h"
#include "Renderer/GraphicsDevice.h"
#include "Renderer/RayTracingScene.h"
#include "Renderer/Material/Material.h"
#include "World/Systems/RenderSceneProxy.h"

#include "Core/Globals.h"
#include "Core/Engine.h"

using namespace Gleam;

void PathTracer::OnCreate(RenderContext& context)
{
	mDevice = context.device;
	mAllocator = context.allocator;
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

	if (mPipelineDirty)
	{
		RayTracingPipelineStateDescriptor pipelineState;
		pipelineState.rayGenerationEntry = "pathTraceRayGen";
		pipelineState.missEntry = "pathTraceMiss";
		pipelineState.maxRecursionDepth = mMaxRayRecursionDepth + 1;
		pipelineState.maxPayloadSize = sizeof(RayPayload);
		pipelineState.maxAttributeSize = sizeof(float2); // float2 barycentrics
		pipelineState.hitGroups = mHitGroups;

		auto handle = mDevice->CreateRayTracingPipeline(pipelineState);
		if (handle.IsValid())
		{
			if (mPathTracingPipeline.IsValid())
			{
				RayTracingPipeline pipeline = mPathTracingPipeline;
				mDevice->Dispose(pipeline);
			}
			mPathTracingPipeline = handle;
		}
		mFrameIndex = 0;
		mPipelineDirty = false;
	}

	auto rtHandle = graph.ImportTexture(mRenderTarget);
	graph.AddComputePass<WorldRenderingData>("PathTracing::Render", [&](RenderGraphBuilder& builder, WorldRenderingData& passData)
	{
		passData.colorTarget = builder.WriteTexture(rtHandle);
		blackboard.Add(passData);
	},
	[this, &sceneData](const CommandBuffer* cmd, const WorldRenderingData& passData)
	{
		if (not mPathTracingPipeline.IsValid())
		{
			return;
		}

		PathTracerConstants constants = {};
		constants.instanceBuffer = sceneData.sceneProxy->GetGlobalInstanceBuffer().GetResourceView();
		constants.accelerationStructure = sceneData.accelerationStructure;
		constants.colorTarget = passData.colorTarget;
		constants.frameIndex = mFrameIndex++;
		constants.maxRayRecursionDepth = mMaxRayRecursionDepth;
		
		cmd->BindRayTracingPipeline(mPathTracingPipeline);
		cmd->SetPushConstant(constants);
		cmd->SetConstantBuffer(sceneData.camera.uniforms, CAMERA_UNIFORMS_BINDING_SLOT);
		cmd->SetConstantBuffer(sceneData.atmosphere.params, SKY_ATMOSPHERE_PARAMS_BINDING_SLOT);
		cmd->SetConstantBuffer(sceneData.atmosphere.uniforms, SKY_ATMOSPHERE_COMMON_UNIFORMS_BINDING_SLOT);
		cmd->DispatchRays((uint32_t)sceneData.camera.uniforms.resolution.x, (uint32_t)sceneData.camera.uniforms.resolution.y, 1u);
	});
}

void PathTracer::RegisterShadingPipeline(const Material* material)
{
	static auto renderSystem = Globals::Engine->GetSubsystem<RenderSystem>();
	auto rayTracingScene = renderSystem->GetRayTracingScene();

	const auto& materialDesc = material->GetDescriptor();
	auto hash = material->GetSurfaceShaderHash();

	const auto& registry = rayTracingScene->GetRegistry();
	uint32_t hitGroupIndex = registry.GetIndex(hash);
	GLEAM_ASSERT(hitGroupIndex != ~0u, "Material is not registered to RayTracingScene.");

	if (hitGroupIndex >= (uint32_t)mHitGroups.size())
	{
		mHitGroups.resize(hitGroupIndex + 1);

		auto& hitGroup = mHitGroups[hitGroupIndex];
		hitGroup.name = materialDesc.surfaceShader;
		hitGroup.closestHitEntry = materialDesc.surfaceShader + "ClosestHit";
		hitGroup.anyHitEntry = materialDesc.surfaceShader + "AnyHit";

		mPipelineDirty = true;
	}
}