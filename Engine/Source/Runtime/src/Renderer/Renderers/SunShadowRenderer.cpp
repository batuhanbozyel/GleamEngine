#include "gpch.h"
#include "SunShadowRenderer.h"
#include "DepthPrepass.h"

#include "Renderer/Mesh.h"
#include "Renderer/CommandBuffer.h"
#include "Renderer/GraphicsDevice.h"
#include "Renderer/RayTracingScene.h"
#include "Renderer/RenderSystem.h"
#include "Renderer/Material/Material.h"

#include "World/Systems/RenderSceneProxy.h"

#include "Core/Globals.h"
#include "Core/Engine.h"

using namespace Gleam;

SunShadowRenderer::SunShadowRenderer()
	: mHitGroupTable(nullptr)
{
	static auto renderSystem = Globals::Engine->GetSubsystem<RenderSystem>();
	auto rayTracingScene = renderSystem->GetRayTracingScene();
	mHitGroupTable = HitGroupTable(rayTracingScene);
}

void SunShadowRenderer::OnCreate(const RenderContext& context)
{
	mDevice = context.device;
}

void SunShadowRenderer::OnDestroy(const RenderContext& context)
{
}

void SunShadowRenderer::AddRenderPasses(RenderGraph& graph, RenderGraphBlackboard& blackboard)
{
	const auto& sceneData = blackboard.Get<SceneRenderingData>();
	const auto& depthPrepassData = blackboard.Get<DepthPrepassData>();
	const auto& sceneTargetDescriptor = graph.GetDescriptor(sceneData.sceneTarget);

	if (mPipelineDirty)
	{
		RayTracingPipelineStateDescriptor pipelineState;
		pipelineState.rayGenerationEntry = "rayTracedSunShadowRayGen";
		pipelineState.missEntries = { "rayTracedSunShadowMiss" };
		pipelineState.maxRecursionDepth = 2;
		pipelineState.maxPayloadSize = sizeof(ShadowPayload);
		pipelineState.maxAttributeSize = sizeof(float2);
		pipelineState.hitGroups = mHitGroupTable.GetDescriptors();

		auto handle = mDevice->CreateRayTracingPipeline(pipelineState);
		if (handle.IsValid())
		{
			if (mRayTracedShadowPipeline.IsValid())
			{
				RayTracingPipeline pipeline = mRayTracedShadowPipeline;
				mDevice->Dispose(pipeline);
			}
			mRayTracedShadowPipeline = handle;
		}
		mPipelineDirty = false;
	}
	
	graph.AddComputePass<SunShadowData>("SunShadowRenderer::RayTracing", [&](RenderGraphBuilder& builder, SunShadowData& passData)
	{
		RenderTextureDescriptor shadowDesc;
		shadowDesc.name = "SunShadowMask";
		shadowDesc.size = Size{
			(float)Math::DivideRoundingUp((uint32_t)sceneTargetDescriptor.size.width,  SHADOW_TILE_WIDTH),
			(float)Math::DivideRoundingUp((uint32_t)sceneTargetDescriptor.size.height, SHADOW_TILE_HEIGHT)
		};
		shadowDesc.format = TextureFormat::R32_UInt;
		passData.shadowMask = builder.WriteTexture(builder.CreateTexture(shadowDesc));
		passData.depthTarget = builder.ReadTexture(depthPrepassData.depthTarget);

		blackboard.Add(passData);
	},
	[this, &sceneData](const CommandBuffer* cmd, const SunShadowData& passData)
	{
		if (not mRayTracedShadowPipeline.IsValid())
		{
			return;
		}

		PathTracerConstants pathTraceConstants = {};
		pathTraceConstants.instanceBuffer = sceneData.sceneProxy->GetGlobalInstanceBuffer().GetResourceView();
		pathTraceConstants.accelerationStructure = sceneData.accelerationStructure;
		pathTraceConstants.colorTarget = passData.shadowMask;
		pathTraceConstants.frameIndex = mFrameIndex++;
		pathTraceConstants.ggxEssTexture = InvalidResourceIndex;
		pathTraceConstants.ggxEAvgTexture = InvalidResourceIndex;
		pathTraceConstants.maxRayRecursionDepth = 2;
		pathTraceConstants.samplesPerPixel = 1;

		RayTracedSunShadowConstants constants = {};
		constants.depthTexture = passData.depthTarget;

		cmd->BindRayTracingPipeline(mRayTracedShadowPipeline);
		cmd->SetPushConstant(constants);
		cmd->SetConstantBuffer(pathTraceConstants, PATH_TRACER_CONSTANTS_BINDING_SLOT);
		cmd->SetConstantBuffer(sceneData.camera.uniforms, CAMERA_UNIFORMS_BINDING_SLOT);
		cmd->SetConstantBuffer(sceneData.atmosphere.uniforms, SKY_ATMOSPHERE_COMMON_UNIFORMS_BINDING_SLOT);
		cmd->DispatchRays((uint32_t)sceneData.camera.uniforms.resolution.x, (uint32_t)sceneData.camera.uniforms.resolution.y, 1u);
	});
}

void SunShadowRenderer::RegisterShadingPipeline(const Material* material)
{
	const auto& materialDesc = material->GetDescriptor();
	auto hash = material->GetSurfaceShaderHash();

	if (not mHitGroupTable.Contains(hash, RayType::PrimaryRay))
	{
		mHitGroupTable.AddPrimaryRay(hash, {
			.name = materialDesc.surfaceShader,
			.closestHitEntry = "",
			.anyHitEntry = materialDesc.surfaceShader + "ShadowAnyHit"
		});
		mPipelineDirty = true;
	}
}
