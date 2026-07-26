#include "gpch.h"
#include "RayTracedReflectionRenderer.h"
#include "BRDFRenderer.h"
#include "DepthPrepass.h"
#include "ReflectionProbeRenderer.h"
#include "GBufferResolveRenderer.h"

#include "Renderer/CommandBuffer.h"
#include "Renderer/GraphicsDevice.h"
#include "Renderer/RayTracingScene.h"
#include "Renderer/RenderSystem.h"
#include "Renderer/Material/Material.h"

#include "World/Systems/RenderSceneProxy.h"

#include "Core/Globals.h"
#include "Core/Engine.h"

using namespace Gleam;

RayTracedReflectionRenderer::RayTracedReflectionRenderer()
	: mHitGroupTable(nullptr)
{
	auto renderSystem = Globals::Engine->GetSubsystem<RenderSystem>();
	auto rayTracingScene = renderSystem->GetRayTracingScene();
	mHitGroupTable = HitGroupTable(rayTracingScene);
}

void RayTracedReflectionRenderer::OnCreate(const RenderContext& context)
{
	mDevice = context.device;
}

void RayTracedReflectionRenderer::AddRenderPasses(RenderGraph& graph, RenderGraphBlackboard& blackboard)
{
	if (mSettings.enable == false || mDevice->GetFeatures().raytracing == false)
	{
		return;
	}

	const auto& brdfData            = blackboard.Get<BRDFData>();
	const auto& sceneData           = blackboard.Get<SceneRenderingData>();
	const auto& gBufferData         = blackboard.Get<GBufferData>();
	const auto& depthPrepassData    = blackboard.Get<DepthPrepassData>();
	const auto& reflectionProbeData = blackboard.Get<ReflectionProbePassData>();
	const auto& sceneTargetDescriptor = graph.GetDescriptor(sceneData.sceneTarget);

	const uint32_t width  = (uint32_t)sceneTargetDescriptor.size.width;
	const uint32_t height = (uint32_t)sceneTargetDescriptor.size.height;

	if (mPipelineDirty)
	{
		RayTracingPipelineStateDescriptor pipelineState;
		pipelineState.rayGenerationEntry = "rayTracedReflectionRayGen";
		pipelineState.missEntries        = { "pathTraceMiss", "pathTraceShadowMiss" };
		pipelineState.maxRecursionDepth  = 2; // reflection ray + shadow
		pipelineState.maxPayloadSize     = sizeof(RayPayload);
		pipelineState.maxAttributeSize   = sizeof(float2);
		pipelineState.hitGroups          = mHitGroupTable.GetDescriptors();

		auto handle = mDevice->CreateRayTracingPipeline(pipelineState);
		if (handle.IsValid())
		{
			if (mRayTracedReflectionPipeline.IsValid())
			{
				RayTracingPipeline pipeline = mRayTracedReflectionPipeline;
				mDevice->Dispose(pipeline);
			}
			mRayTracedReflectionPipeline = handle;
		}
		mPipelineDirty = false;
	}

	struct ReflectionPassData
	{
		TextureHandle depth;
		TextureHandle shadingNormal;
		TextureHandle geometryNormal;
		TextureHandle roughness;
		TextureHandle brdfLut;
		TextureHandle ggxEssLut;
		TextureHandle ggxEAvgLut;
		TextureHandle diffuseReflection;
		TextureHandle specularReflection;
		TextureHandle reflectionTarget;
	};

	auto& reflectionData = graph.AddComputePass<ReflectionPassData>("RayTracedReflectionRenderer::RayTracing",
	[&](RenderGraphBuilder& builder, ReflectionPassData& passData)
	{
		RenderTextureDescriptor reflectionDesc;
		reflectionDesc.name   = "RayTraced Reflection";
		reflectionDesc.size   = sceneTargetDescriptor.size;
		reflectionDesc.format = TextureFormat::R16G16B16A16_SFloat;
		passData.reflectionTarget = builder.WriteTexture(builder.CreateTexture(reflectionDesc));

		passData.depth              = builder.ReadTexture(depthPrepassData.depthTarget);
		passData.shadingNormal      = builder.ReadTexture(gBufferData.shadingNormalTarget);
		passData.geometryNormal     = builder.ReadTexture(gBufferData.geometryNormalTarget);
		passData.roughness          = builder.ReadTexture(gBufferData.roughnessTarget);
		passData.brdfLut            = builder.ReadTexture(brdfData.brdfLut);
		passData.ggxEssLut          = builder.ReadTexture(brdfData.ggxEssLut);
		passData.ggxEAvgLut         = builder.ReadTexture(brdfData.ggxEAvgLut);
		passData.diffuseReflection  = builder.ReadTexture(reflectionProbeData.diffuseReflection);
		passData.specularReflection = builder.ReadTexture(reflectionProbeData.specularReflection);
	},
	[this, &sceneData, width, height](const CommandBuffer* cmd, const ReflectionPassData& passData)
	{
		if (not mRayTracedReflectionPipeline.IsValid())
		{
			return;
		}

		PathTracerConstants pathTraceConstants = {};
		pathTraceConstants.instanceBuffer        = sceneData.sceneProxy->GetGlobalInstanceBuffer().GetResourceView();
		pathTraceConstants.accelerationStructure = sceneData.accelerationStructure;
		pathTraceConstants.colorTarget           = passData.reflectionTarget;
		pathTraceConstants.ggxEssTexture         = passData.ggxEssLut;
		pathTraceConstants.ggxEAvgTexture        = passData.ggxEAvgLut;
		pathTraceConstants.frameIndex            = mFrameIndex;
		pathTraceConstants.sceneTarget           = InvalidResourceIndex;
		pathTraceConstants.maxRayRecursionDepth  = 2;
		pathTraceConstants.samplesPerPixel       = 1;

		RayTracedReflectionConstants constants = {};
		constants.depthTexture              = passData.depth;
		constants.shadingNormalTexture      = passData.shadingNormal;
		constants.geometryNormalTexture     = passData.geometryNormal;
		constants.roughnessTexture          = passData.roughness;
		constants.brdfTexture               = passData.brdfLut;
		constants.diffuseReflectionTexture  = passData.diffuseReflection;
		constants.specularReflectionTexture = passData.specularReflection;
		constants.roughnessCutoff           = mSettings.roughnessCutoff;

		cmd->BindRayTracingPipeline(mRayTracedReflectionPipeline);
		cmd->SetPushConstant(constants);
		cmd->SetConstantBuffer(pathTraceConstants, PATH_TRACER_CONSTANTS_BINDING_SLOT);
		cmd->SetConstantBuffer(sceneData.camera.uniforms, CAMERA_UNIFORMS_BINDING_SLOT);
		cmd->SetConstantBuffer(sceneData.atmosphere.params, SKY_ATMOSPHERE_PARAMS_BINDING_SLOT);
		cmd->SetConstantBuffer(sceneData.atmosphere.uniforms, SKY_ATMOSPHERE_COMMON_UNIFORMS_BINDING_SLOT);
		cmd->DispatchRays(width, height, 1u);
	});

	RayTracedReflectionData output;
	output.reflectionTarget = reflectionData.reflectionTarget;
	blackboard.Add(output);

	mFrameIndex++;
}

void RayTracedReflectionRenderer::SetSettings(const RayTracedReflectionSettings& settings)
{
	if (memcmp(&mSettings, &settings, sizeof(RayTracedReflectionSettings)) != 0)
	{
		mSettings = settings;
		mFrameIndex = 0;
	}
}

void RayTracedReflectionRenderer::RegisterShadingPipeline(const Material* material)
{
	const auto& materialDesc = material->GetDescriptor();
	auto hash = material->GetSurfaceShaderHash();

	if (not mHitGroupTable.Contains(hash, RayType::PrimaryRay))
	{
		mHitGroupTable.AddPrimaryRay(hash, {
				.name = materialDesc.surfaceShader,
				.closestHitEntry = materialDesc.surfaceShader + "ReflectionClosestHit",
				.anyHitEntry = materialDesc.surfaceShader + "ReflectionAnyHit"
		});
		mPipelineDirty = true;
	}

	if (not mHitGroupTable.Contains(hash, RayType::ShadowRay))
	{
		mHitGroupTable.AddShadowRay(hash, {
				.name = materialDesc.surfaceShader,
				.closestHitEntry = "",
				.anyHitEntry = materialDesc.surfaceShader + "ShadowAnyHit"
		});
		mPipelineDirty = true;
	}
}