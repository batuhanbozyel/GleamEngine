#include "gpch.h"
#include "PathTracer.h"

#include "BRDFRenderer.h"
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

PathTracer::PathTracer()
	: mHitGroupTable(nullptr)
{
	static auto renderSystem = Globals::Engine->GetSubsystem<RenderSystem>();
	auto rayTracingScene = renderSystem->GetRayTracingScene();
	mHitGroupTable = HitGroupTable(rayTracingScene);
}

void PathTracer::OnCreate(const RenderContext& context)
{
	mDevice = context.device;
	mAllocator = context.allocator;
}

void PathTracer::OnDestroy(const RenderContext& context)
{
	if (mRenderTarget.IsValid())
	{
		mDevice->Dispose(mAllocator, mRenderTarget, BarrierStage::None);
	}
}

void PathTracer::AddRenderPasses(RenderGraph& graph, RenderGraphBlackboard& blackboard)
{
	const auto& brdfData = blackboard.Get<BRDFData>();
	const auto& sceneData = blackboard.Get<SceneRenderingData>();
	const auto& sceneTargetDescriptor = graph.GetDescriptor(sceneData.sceneTarget);

	if (mRenderTarget.GetDescriptor().size != sceneTargetDescriptor.size)
	{
		mFrameIndex = 0;
		if (mRenderTarget.IsValid())
		{
			mDevice->Dispose(mAllocator, mRenderTarget, BarrierStage::None);
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
		pipelineState.missEntries = {"pathTraceMiss", "pathTraceShadowMiss"};
		pipelineState.maxRecursionDepth = mSettings.maxRayRecursionDepth + 2; // +1 for shadow rays, +1 for ray generation
		pipelineState.maxPayloadSize = sizeof(RayPayload);
		pipelineState.maxAttributeSize = sizeof(float2); // float2 barycentrics
		pipelineState.hitGroups = mHitGroupTable.GetDescriptors();

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
		passData.ggxEssLut = builder.ReadTexture(brdfData.ggxEssLut);
		passData.ggxEAvgLut = builder.ReadTexture(brdfData.ggxEAvgLut);
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
		constants.ggxEssTexture = passData.ggxEssLut;
		constants.ggxEAvgTexture = passData.ggxEAvgLut;
		constants.maxRayRecursionDepth = mSettings.maxRayRecursionDepth;
		constants.samplesPerPixel = mSettings.samplesPerPixel;
		
		cmd->BindRayTracingPipeline(mPathTracingPipeline);
		cmd->SetPushConstant(constants);
		cmd->SetConstantBuffer(sceneData.camera.uniforms, CAMERA_UNIFORMS_BINDING_SLOT);
		cmd->SetConstantBuffer(sceneData.atmosphere.params, SKY_ATMOSPHERE_PARAMS_BINDING_SLOT);
		cmd->SetConstantBuffer(sceneData.atmosphere.uniforms, SKY_ATMOSPHERE_COMMON_UNIFORMS_BINDING_SLOT);
		cmd->DispatchRays((uint32_t)sceneData.camera.uniforms.resolution.x, (uint32_t)sceneData.camera.uniforms.resolution.y, 1u);
	});
}

void PathTracer::SetSettings(const PathTracerSettings& settings)
{
	if (mSettings.maxRayRecursionDepth != settings.maxRayRecursionDepth)
	{
		mPipelineDirty = true;
	}

	if (mSettings.samplesPerPixel != settings.samplesPerPixel)
	{
		mFrameIndex = 0;
	}
	mSettings = settings;
}

void PathTracer::RegisterShadingPipeline(const Material* material)
{
	const auto& materialDesc = material->GetDescriptor();
	auto hash = material->GetSurfaceShaderHash();

	if (not mHitGroupTable.Contains(hash, RayType::PrimaryRay))
	{
		mHitGroupTable.AddPrimaryRay(hash, { .name = materialDesc.surfaceShader, .closestHitEntry = materialDesc.surfaceShader + "ClosestHit", .anyHitEntry = materialDesc.surfaceShader + "AnyHit" });
		mPipelineDirty = true;
	}

	if (not mHitGroupTable.Contains(hash, RayType::ShadowRay))
	{
		mHitGroupTable.AddShadowRay(hash, { .name = materialDesc.surfaceShader, .closestHitEntry = {}, .anyHitEntry = materialDesc.surfaceShader + "ShadowAnyHit" });
		mPipelineDirty = true;
	}
}
