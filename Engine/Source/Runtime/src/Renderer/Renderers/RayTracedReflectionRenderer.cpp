#include "gpch.h"
#include "RayTracedReflectionRenderer.h"
#include "BRDFRenderer.h"
#include "DepthPrepass.h"
#include "ReflectionProbeRenderer.h"
#include "GBufferResolveRenderer.h"

#include "Renderer/SceneView.h"
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

void RayTracedReflectionHistory::ReleasePersistentTextures(GraphicsDevice* device, GPUAllocator* allocator)
{
	for (uint32_t i = 0; i < 2; ++i)
	{
		if (radiance[i].IsValid())
		{
			device->Dispose(allocator, radiance[i], BarrierStage::None);
		}

		if (variance[i].IsValid())
		{
			device->Dispose(allocator, variance[i], BarrierStage::None);
		}
	}
}

void RayTracedReflectionHistory::ReleaseDenoiserTextures(GraphicsDevice* device, GPUAllocator* allocator)
{
	for (uint32_t i = 0; i < 2; ++i)
	{
		if (sampleCount[i].IsValid())
		{
			device->Dispose(allocator, sampleCount[i], BarrierStage::None);
		}
	}

	if (averageRadiance.IsValid())
	{
		device->Dispose(allocator, averageRadiance, BarrierStage::None);
	}
}

void RayTracedReflectionHistory::Release(GraphicsDevice* device, GPUAllocator* allocator)
{
	ReleasePersistentTextures(device, allocator);
	ReleaseDenoiserTextures(device, allocator);
}

void RayTracedReflectionRenderer::OnCreate(const RenderContext& context)
{
	mDevice    = context.device;

	ComputePipelineStateDescriptor classificationDesc;
	classificationDesc.entryPoint = "reflectionClassification";
	mClassificationPipeline = context.device->CreateComputePipeline(classificationDesc);

	ComputePipelineStateDescriptor prepareDispatchArgsDesc;
	prepareDispatchArgsDesc.entryPoint = "prepareReflectionDispatchArgs";
	mPrepareDispatchArgsPipeline = context.device->CreateComputePipeline(prepareDispatchArgsDesc);

	ComputePipelineStateDescriptor reprojectDesc;
	reprojectDesc.entryPoint = "reflectionDenoiserReproject";
	mReprojectPipeline = context.device->CreateComputePipeline(reprojectDesc);

	ComputePipelineStateDescriptor prefilterDesc;
	prefilterDesc.entryPoint = "reflectionDenoiserPrefilter";
	mPrefilterPipeline = context.device->CreateComputePipeline(prefilterDesc);

	ComputePipelineStateDescriptor resolveTemporalDesc;
	resolveTemporalDesc.entryPoint = "reflectionDenoiserResolveTemporal";
	mResolveTemporalPipeline = context.device->CreateComputePipeline(resolveTemporalDesc);
}

void RayTracedReflectionRenderer::CreatePersistentTextures(RayTracedReflectionHistory& history, GPUAllocator* allocator, const Size& size) const
{
	if (not history.radiance[0].IsValid() or history.radiance[0].GetDescriptor().size != size)
	{
		history.ReleasePersistentTextures(mDevice, allocator);

		RenderTextureDescriptor textureDesc;
		textureDesc.dimension = TextureDimension::Texture2D;
		textureDesc.size      = size;

		textureDesc.format = TextureFormat::R16G16B16A16_SFloat;
		textureDesc.name   = "RayTracedReflectionRenderer::ReflectionDenoiser::Radiance 0";
		history.radiance[0] = mDevice->CreateTexture(allocator, textureDesc);
		textureDesc.name   = "RayTracedReflectionRenderer::ReflectionDenoiser::Radiance 1";
		history.radiance[1] = mDevice->CreateTexture(allocator, textureDesc);

		textureDesc.format = TextureFormat::R16_SFloat;
		textureDesc.name   = "RayTracedReflectionRenderer::ReflectionDenoiser::Variance 0";
		history.variance[0] = mDevice->CreateTexture(allocator, textureDesc);
		textureDesc.name   = "RayTracedReflectionRenderer::ReflectionDenoiser::Variance 1";
		history.variance[1] = mDevice->CreateTexture(allocator, textureDesc);

		history.firstFrame = true;
	}
}

void RayTracedReflectionRenderer::CreateDenoiserTextures(RayTracedReflectionHistory& history, GPUAllocator* allocator, const Size& size) const
{
	if (not history.sampleCount[0].IsValid() or history.sampleCount[0].GetDescriptor().size != size)
	{
		history.ReleaseDenoiserTextures(mDevice, allocator);

		RenderTextureDescriptor textureDesc;
		textureDesc.dimension = TextureDimension::Texture2D;
		textureDesc.size      = size;

		textureDesc.format = TextureFormat::R16_SFloat;
		textureDesc.name   = "RayTracedReflectionRenderer::ReflectionDenoiser::SampleCount 0";
		history.sampleCount[0] = mDevice->CreateTexture(allocator, textureDesc);
		textureDesc.name   = "RayTracedReflectionRenderer::ReflectionDenoiser::SampleCount 1";
		history.sampleCount[1] = mDevice->CreateTexture(allocator, textureDesc);

		textureDesc.format 	= TextureFormat::R11G11B10_SFloat;
		textureDesc.name  	= "RayTracedReflectionRenderer::ReflectionDenoiser::AverageRadiance";
		textureDesc.size 	= Size(
			float(Math::DivideRoundingUp((uint32_t)size.width, REFLECTION_DENOISER_TILE_SIZE)),
			float(Math::DivideRoundingUp((uint32_t)size.height, REFLECTION_DENOISER_TILE_SIZE)));
		history.averageRadiance = mDevice->CreateTexture(allocator, textureDesc);

		history.firstFrame = true;
	}
}

void RayTracedReflectionRenderer::AddRenderPasses(RenderGraph& graph, RenderGraphBlackboard& blackboard)
{
	const auto& brdfData            = blackboard.Get<BRDFData>();
	const auto& sceneData           = blackboard.Get<SceneRenderingData>();
	if (not sceneData.world)
	{
		return;
	}

	auto& history = sceneData.view->GetHistory<RayTracedReflectionHistory>();
	const bool enabled = mSettings.enable and mDevice->GetFeatures().raytracing;
	if (not enabled or history.version != mHistoryVersion)
	{
		history.Release(mDevice, graph.GetAllocator());
		history.frameIndex = 0;
		history.firstFrame = true;
		history.version = mHistoryVersion;
	}

	if (not enabled)
	{
		return;
	}

	const auto& gBufferData         = blackboard.Get<GBufferData>();
	const auto& depthPrepassData    = blackboard.Get<DepthPrepassData>();
	const auto& reflectionProbeData = blackboard.Get<ReflectionProbePassData>();
	const auto& sceneTargetDescriptor = graph.GetDescriptor(sceneData.sceneTarget);

	const uint32_t width  = (uint32_t)sceneTargetDescriptor.size.width;
	const uint32_t height = (uint32_t)sceneTargetDescriptor.size.height;

	const uint32_t numTilesX = Math::DivideRoundingUp(width, REFLECTION_DENOISER_TILE_SIZE);
	const uint32_t numTilesY = Math::DivideRoundingUp(height, REFLECTION_DENOISER_TILE_SIZE);
	const uint32_t numTiles  = numTilesX * numTilesY;

	const float roughnessThreshold = mSettings.roughnessCutoff * mSettings.roughnessCutoff;

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
		++mHistoryVersion;
		history.frameIndex = 0;
		history.firstFrame = true;
		history.version = mHistoryVersion;
	}
	CreatePersistentTextures(history, graph.GetAllocator(), sceneTargetDescriptor.size);

	// ----------------------------------------------------------------
	// Import persistent denoiser textures + first frame history clear
	// ----------------------------------------------------------------
	const uint32_t currIndex = history.frameIndex & 1u;
	const uint32_t prevIndex = currIndex ^ 1u;

	ImportResourceParams importParams;
	importParams.initialLayout = history.firstFrame ? BarrierLayout::Undefined : BarrierLayout::ShaderResource;
	importParams.finalLayout = BarrierLayout::ShaderResource;
	importParams.clearOnFirstUse = history.firstFrame;

	TextureHandle radiance               = graph.ImportTexture(history.radiance[currIndex], importParams);
	TextureHandle radianceHistory        = graph.ImportTexture(history.radiance[prevIndex], importParams);
	TextureHandle variance               = graph.ImportTexture(history.variance[currIndex], importParams);
	TextureHandle varianceHistory        = graph.ImportTexture(history.variance[prevIndex], importParams);

	// ----------------------------------------------------------------
	// Pass 0 — Ray classification
	// ----------------------------------------------------------------
	struct ClassificationPassData
	{
		TextureHandle depth;
		TextureHandle normal;
		TextureHandle roughness;
		TextureHandle motionVector;
		TextureHandle varianceHistory;
		TextureHandle specularReflection;
		TextureHandle radiance;
		BufferHandle  rayList;
		BufferHandle  denoiserTileList;
		BufferHandle  rayCounter;
	};

	auto& classificationData = graph.AddComputePass<ClassificationPassData>("RayTracedReflectionRenderer::Classification",
	[&](RenderGraphBuilder& builder, ClassificationPassData& passData)
	{
		BufferDescriptor rayListDesc;
		rayListDesc.name = "Reflection Ray List";
		rayListDesc.size = width * height * sizeof(uint32_t);
		passData.rayList = builder.WriteBuffer(builder.CreateBuffer(rayListDesc));

		BufferDescriptor tileListDesc;
		tileListDesc.name = "Reflection Denoiser Tile List";
		tileListDesc.size = numTiles * sizeof(uint32_t);
		passData.denoiserTileList = builder.WriteBuffer(builder.CreateBuffer(tileListDesc));

		BufferDescriptor rayCounterDesc;
		rayCounterDesc.name = "Reflection Ray Counter";
		rayCounterDesc.size = REFLECTION_RAY_COUNTER_SLOTS * sizeof(uint32_t);
		passData.rayCounter = builder.WriteBuffer(builder.CreateBuffer(rayCounterDesc));

		passData.radiance           = builder.WriteTexture(radiance);
		passData.depth              = builder.ReadTexture(depthPrepassData.depthTarget);
		passData.normal             = builder.ReadTexture(gBufferData.shadingNormalTarget);
		passData.roughness          = builder.ReadTexture(gBufferData.roughnessTarget);
		passData.motionVector       = builder.ReadTexture(gBufferData.motionVectorTarget);
		passData.varianceHistory    = builder.ReadTexture(varianceHistory);
		passData.specularReflection = builder.ReadTexture(reflectionProbeData.specularReflection);
	},
	[this, &sceneData, roughnessThreshold, numTilesX, numTilesY, frameIndex = history.frameIndex](const CommandBuffer* cmd, const ClassificationPassData& passData)
	{
		cmd->ClearBuffer(passData.rayCounter);

		Buffer rayCounter = passData.rayCounter;
		BarrierGroup clearBarrier;
		clearBarrier.bufferBarriers.push_back({
			.resource  = rayCounter.GetHandle(),
			.srcStage  = BarrierStage::ClearUnorderedAccess,
			.dstStage  = BarrierStage::ComputeShading,
			.srcAccess = BarrierAccess::UnorderedAccess,
			.dstAccess = BarrierAccess::UnorderedAccess,
		});
		cmd->Barrier(clearBarrier);

		ReflectionClassificationConstants constants = {};
		constants.depthTexture                 = passData.depth;
		constants.normalTexture                = passData.normal;
		constants.roughnessTexture             = passData.roughness;
		constants.motionVectorTexture          = passData.motionVector;
		constants.varianceHistoryTexture       = passData.varianceHistory;
		constants.specularReflectionTexture    = passData.specularReflection;
		constants.radianceTexture              = passData.radiance;
		constants.rayListBuffer                = passData.rayList;
		constants.denoiserTileListBuffer       = passData.denoiserTileList;
		constants.rayCounterBuffer             = passData.rayCounter;
		constants.roughnessThreshold           = roughnessThreshold;
		constants.varianceThreshold            = mSettings.varianceThreshold;
		constants.samplesPerQuad               = mSettings.samplesPerQuad;
		constants.frameIndex                   = frameIndex;
		constants.temporalVarianceGuidedTracing = mSettings.temporalVarianceGuidedTracing ? 1u : 0u;

		cmd->BindComputePipeline(mClassificationPipeline);
		cmd->SetPushConstant(constants);
		cmd->SetConstantBuffer(sceneData.camera.uniforms, CAMERA_UNIFORMS_BINDING_SLOT);
		cmd->Dispatch(numTilesX, numTilesY, 1u);
	});

	// ----------------------------------------------------------------
	// Pass 1 — Prepare indirect ray and denoiser dispatch arguments
	// ----------------------------------------------------------------
	struct PrepareDispatchArgsPassData
	{
		BufferHandle rayCounter;
		BufferHandle rayDispatchArgs;
		BufferHandle denoiserDispatchArgs;
	};

	auto& prepareArgsData = graph.AddComputePass<PrepareDispatchArgsPassData>("RayTracedReflectionRenderer::PrepareDispatchArgs",
	[&](RenderGraphBuilder& builder, PrepareDispatchArgsPassData& passData)
	{
		BufferDescriptor dispatchArgsDesc;
		dispatchArgsDesc.name  = "Reflection Ray Dispatch Args";
		dispatchArgsDesc.size  = sizeof(DispatchIndirectArguments);
		dispatchArgsDesc.usage = BufferUsage::IndirectArgument;
		passData.rayDispatchArgs = builder.WriteBuffer(builder.CreateBuffer(dispatchArgsDesc));

		dispatchArgsDesc.name = "Reflection Denoiser Dispatch Args";
		passData.denoiserDispatchArgs = builder.WriteBuffer(builder.CreateBuffer(dispatchArgsDesc));

		passData.rayCounter = builder.WriteBuffer(classificationData.rayCounter);
	},
	[this](const CommandBuffer* cmd, const PrepareDispatchArgsPassData& passData)
	{
		PrepareReflectionDispatchArgsConstants constants = {};
		constants.rayCounterBuffer           = passData.rayCounter;
		constants.rayDispatchArgsBuffer      = passData.rayDispatchArgs;
		constants.denoiserDispatchArgsBuffer = passData.denoiserDispatchArgs;

		cmd->BindComputePipeline(mPrepareDispatchArgsPipeline);
		cmd->SetPushConstant(constants);
		cmd->Dispatch(1u, 1u, 1u);
	});

	// ----------------------------------------------------------------
	// Pass 2 — Ray traced reflections
	// ----------------------------------------------------------------
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
		BufferHandle  rayList;
		BufferHandle  dispatchArgs;
	};

	auto& reflectionData = graph.AddComputePass<ReflectionPassData>("RayTracedReflectionRenderer::RayTracing",
	[&](RenderGraphBuilder& builder, ReflectionPassData& passData)
	{
		passData.reflectionTarget = builder.WriteTexture(classificationData.radiance);

		passData.depth              = builder.ReadTexture(depthPrepassData.depthTarget);
		passData.shadingNormal      = builder.ReadTexture(gBufferData.shadingNormalTarget);
		passData.geometryNormal     = builder.ReadTexture(gBufferData.geometryNormalTarget);
		passData.roughness          = builder.ReadTexture(gBufferData.roughnessTarget);
		passData.brdfLut            = builder.ReadTexture(brdfData.brdfLut);
		passData.ggxEssLut          = builder.ReadTexture(brdfData.ggxEssLut);
		passData.ggxEAvgLut         = builder.ReadTexture(brdfData.ggxEAvgLut);
		passData.diffuseReflection  = builder.ReadTexture(reflectionProbeData.diffuseReflection);
		passData.specularReflection = builder.ReadTexture(reflectionProbeData.specularReflection);
		passData.rayList            = builder.ReadBuffer(classificationData.rayList);
		passData.dispatchArgs       = builder.ReadBuffer(prepareArgsData.rayDispatchArgs);
	},
	[this, &sceneData, frameIndex = history.frameIndex](const CommandBuffer* cmd, const ReflectionPassData& passData)
	{
		if (not mRayTracedReflectionPipeline.IsValid())
		{
			return;
		}

		PathTracerConstants pathTraceConstants = {};
		pathTraceConstants.instanceBuffer        = sceneData.sceneProxy->GetInstanceBuffer().GetResourceView();
		pathTraceConstants.accelerationStructure = sceneData.accelerationStructure;
		pathTraceConstants.colorTarget           = passData.reflectionTarget;
		pathTraceConstants.ggxEssTexture         = passData.ggxEssLut;
		pathTraceConstants.ggxEAvgTexture        = passData.ggxEAvgLut;
		pathTraceConstants.frameIndex            = frameIndex;
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
		constants.rayListBuffer             = passData.rayList;

		cmd->BindRayTracingPipeline(mRayTracedReflectionPipeline);
		cmd->SetPushConstant(constants);
		cmd->SetConstantBuffer(pathTraceConstants, PATH_TRACER_CONSTANTS_BINDING_SLOT);
		cmd->SetConstantBuffer(sceneData.camera.uniforms, CAMERA_UNIFORMS_BINDING_SLOT);
		cmd->SetConstantBuffer(sceneData.atmosphere.params, SKY_ATMOSPHERE_PARAMS_BINDING_SLOT);
		cmd->SetConstantBuffer(sceneData.atmosphere.uniforms, SKY_ATMOSPHERE_COMMON_UNIFORMS_BINDING_SLOT);
		cmd->DispatchRaysIndirect(passData.dispatchArgs);
	});

	TextureHandle reflectionTarget = reflectionData.reflectionTarget;

	if (mSettings.denoise)
	{
		CreateDenoiserTextures(history, graph.GetAllocator(), sceneTargetDescriptor.size);
		importParams.initialLayout = history.firstFrame ? BarrierLayout::Undefined : BarrierLayout::ShaderResource;
		importParams.clearOnFirstUse = history.firstFrame;

		TextureHandle sampleCount            = graph.ImportTexture(history.sampleCount[currIndex], importParams);
		TextureHandle sampleCountHistory     = graph.ImportTexture(history.sampleCount[prevIndex], importParams);
		TextureHandle averageRadiance		 = graph.ImportTexture(history.averageRadiance, importParams);

		if (history.firstFrame)
		{
			struct ClearHistoryPassData
			{
			};

			graph.AddRenderPass<ClearHistoryPassData>("RayTracedReflectionRenderer::ClearHistory",
			[&](RenderGraphBuilder& builder, ClearHistoryPassData& passData)
			{
				sampleCount            = builder.UseColorBuffer(sampleCount);
				sampleCountHistory     = builder.UseColorBuffer(sampleCountHistory);
				averageRadiance		   = builder.UseColorBuffer(averageRadiance);
			},
			[](const CommandBuffer* cmd, const ClearHistoryPassData& passData)
			{
				// Attachments are cleared by the load action on render pass begin
			});
		}
		
		// ----------------------------------------------------------------
		// Pass 3 — Reproject
		// ----------------------------------------------------------------
		struct ReprojectPassData
		{
			TextureHandle depth;
			TextureHandle normal;
			TextureHandle roughness;
			TextureHandle motionVector;
			TextureHandle previousDepth;
			TextureHandle previousNormal;
			TextureHandle previousRoughness;
			TextureHandle radiance;
			TextureHandle radianceHistory;
			TextureHandle variance;
			TextureHandle sampleCount;
			TextureHandle varianceOutput;
			TextureHandle sampleCountOutput;
			TextureHandle averageRadianceOutput;
			TextureHandle reprojectedRadiance;
			BufferHandle  tileList;
			BufferHandle  dispatchArgs;
		};

		auto& reprojectData = graph.AddComputePass<ReprojectPassData>("RayTracedReflectionRenderer::Reproject",
		[&](RenderGraphBuilder& builder, ReprojectPassData& passData)
		{
			RenderTextureDescriptor reprojectedDesc;
			reprojectedDesc.name   = "Reflection Reprojected Radiance";
			reprojectedDesc.size   = sceneTargetDescriptor.size;
			reprojectedDesc.format = TextureFormat::R16G16B16A16_SFloat;
			passData.reprojectedRadiance = builder.WriteTexture(builder.CreateTexture(reprojectedDesc));

			passData.depth                 = builder.ReadTexture(depthPrepassData.depthTarget);
			passData.normal                = builder.ReadTexture(gBufferData.shadingNormalTarget);
			passData.roughness             = builder.ReadTexture(gBufferData.roughnessTarget);
			passData.motionVector          = builder.ReadTexture(gBufferData.motionVectorTarget);
			passData.previousDepth         = builder.ReadTexture(depthPrepassData.previousDepth);
			passData.previousNormal        = builder.ReadTexture(gBufferData.previousShadingNormalTarget);
			passData.previousRoughness     = builder.ReadTexture(gBufferData.previousRoughnessTarget);
			passData.radiance              = builder.ReadTexture(reflectionTarget);
			passData.radianceHistory       = builder.ReadTexture(radianceHistory);
			passData.variance              = builder.ReadTexture(varianceHistory);
			passData.sampleCount           = builder.ReadTexture(sampleCountHistory);
			passData.varianceOutput        = builder.WriteTexture(variance);
			passData.sampleCountOutput     = builder.WriteTexture(sampleCount);
			passData.averageRadianceOutput = builder.WriteTexture(averageRadiance);
			passData.tileList              = builder.ReadBuffer(classificationData.denoiserTileList);
			passData.dispatchArgs          = builder.ReadBuffer(prepareArgsData.denoiserDispatchArgs);
		},
		[this, &sceneData, roughnessThreshold](const CommandBuffer* cmd, const ReprojectPassData& passData)
		{
			ReflectionDenoiserReprojectConstants constants = {};
			constants.depthTexture                 = passData.depth;
			constants.normalTexture                = passData.normal;
			constants.roughnessTexture             = passData.roughness;
			constants.motionVectorTexture          = passData.motionVector;
			constants.previousDepthTexture         = passData.previousDepth;
			constants.previousNormalTexture        = passData.previousNormal;
			constants.previousRoughnessTexture     = passData.previousRoughness;
			constants.radianceTexture              = passData.radiance;
			constants.radianceHistoryTexture       = passData.radianceHistory;
			constants.varianceTexture              = passData.variance;
			constants.sampleCountTexture           = passData.sampleCount;
			constants.tileListBuffer               = passData.tileList;
			constants.varianceOutputTexture        = passData.varianceOutput;
			constants.sampleCountOutputTexture     = passData.sampleCountOutput;
			constants.averageRadianceOutputTexture = passData.averageRadianceOutput;
			constants.reprojectedRadianceTexture   = passData.reprojectedRadiance;
			constants.roughnessThreshold           = roughnessThreshold;
			constants.temporalStabilityFactor      = mSettings.temporalStability;

			cmd->BindComputePipeline(mReprojectPipeline);
			cmd->SetPushConstant(constants);
			cmd->SetConstantBuffer(sceneData.camera.uniforms, CAMERA_UNIFORMS_BINDING_SLOT);
			cmd->DispatchIndirect(passData.dispatchArgs);
		});

		// ----------------------------------------------------------------
		// Pass 4 — Prefilter
		// ----------------------------------------------------------------
		struct PrefilterPassData
		{
			TextureHandle depth;
			TextureHandle normal;
			TextureHandle roughness;
			TextureHandle radiance;
			TextureHandle variance;
			TextureHandle averageRadiance;
			TextureHandle radianceOutput;
			TextureHandle varianceOutput;
			BufferHandle  tileList;
			BufferHandle  dispatchArgs;
		};

		auto& prefilterData = graph.AddComputePass<PrefilterPassData>("RayTracedReflectionRenderer::Prefilter",
		[&](RenderGraphBuilder& builder, PrefilterPassData& passData)
		{
			passData.depth           = builder.ReadTexture(depthPrepassData.depthTarget);
			passData.normal          = builder.ReadTexture(gBufferData.shadingNormalTarget);
			passData.roughness       = builder.ReadTexture(gBufferData.roughnessTarget);
			passData.radiance        = builder.ReadTexture(reflectionTarget);
			passData.variance        = builder.ReadTexture(reprojectData.varianceOutput);
			passData.averageRadiance = builder.ReadTexture(reprojectData.averageRadianceOutput);
			passData.radianceOutput  = builder.WriteTexture(reprojectData.radianceHistory);
			passData.varianceOutput  = builder.WriteTexture(reprojectData.variance);
			passData.tileList        = builder.ReadBuffer(reprojectData.tileList);
			passData.dispatchArgs    = builder.ReadBuffer(reprojectData.dispatchArgs);
		},
		[this, &sceneData, roughnessThreshold](const CommandBuffer* cmd, const PrefilterPassData& passData)
		{
			ReflectionDenoiserPrefilterConstants constants = {};
			constants.depthTexture           = passData.depth;
			constants.normalTexture          = passData.normal;
			constants.roughnessTexture       = passData.roughness;
			constants.radianceTexture        = passData.radiance;
			constants.varianceTexture        = passData.variance;
			constants.averageRadianceTexture = passData.averageRadiance;
			constants.tileListBuffer         = passData.tileList;
			constants.radianceOutputTexture  = passData.radianceOutput;
			constants.varianceOutputTexture  = passData.varianceOutput;
			constants.roughnessThreshold     = roughnessThreshold;

			cmd->BindComputePipeline(mPrefilterPipeline);
			cmd->SetPushConstant(constants);
			cmd->SetConstantBuffer(sceneData.camera.uniforms, CAMERA_UNIFORMS_BINDING_SLOT);
			cmd->DispatchIndirect(passData.dispatchArgs);
		});

		// ----------------------------------------------------------------
		// Pass 5 — Resolve temporal
		// ----------------------------------------------------------------
		struct ResolveTemporalPassData
		{
			TextureHandle roughness;
			TextureHandle radiance;
			TextureHandle variance;
			TextureHandle sampleCount;
			TextureHandle averageRadiance;
			TextureHandle reprojectedRadiance;
			TextureHandle radianceOutput;
			TextureHandle varianceOutput;
			BufferHandle  tileList;
			BufferHandle  dispatchArgs;
		};

		auto& resolveData = graph.AddComputePass<ResolveTemporalPassData>("RayTracedReflectionRenderer::ResolveTemporal",
		[&](RenderGraphBuilder& builder, ResolveTemporalPassData& passData)
		{
			passData.roughness           = builder.ReadTexture(gBufferData.roughnessTarget);
			passData.radiance            = builder.ReadTexture(prefilterData.radianceOutput);
			passData.variance            = builder.ReadTexture(prefilterData.varianceOutput);
			passData.sampleCount         = builder.ReadTexture(reprojectData.sampleCountOutput);
			passData.averageRadiance     = builder.ReadTexture(prefilterData.averageRadiance);
			passData.reprojectedRadiance = builder.ReadTexture(reprojectData.reprojectedRadiance);
			passData.radianceOutput      = builder.WriteTexture(prefilterData.radiance);
			passData.varianceOutput      = builder.WriteTexture(prefilterData.variance);
			passData.tileList            = builder.ReadBuffer(prefilterData.tileList);
			passData.dispatchArgs        = builder.ReadBuffer(prefilterData.dispatchArgs);
		},
		[this, &sceneData, roughnessThreshold](const CommandBuffer* cmd, const ResolveTemporalPassData& passData)
		{
			ReflectionDenoiserResolveTemporalConstants constants = {};
			constants.roughnessTexture           = passData.roughness;
			constants.radianceTexture            = passData.radiance;
			constants.varianceTexture            = passData.variance;
			constants.sampleCountTexture         = passData.sampleCount;
			constants.averageRadianceTexture     = passData.averageRadiance;
			constants.reprojectedRadianceTexture = passData.reprojectedRadiance;
			constants.tileListBuffer             = passData.tileList;
			constants.radianceOutputTexture      = passData.radianceOutput;
			constants.varianceOutputTexture      = passData.varianceOutput;
			constants.roughnessThreshold         = roughnessThreshold;
			constants.temporalStabilityFactor    = mSettings.temporalStability;

			cmd->BindComputePipeline(mResolveTemporalPipeline);
			cmd->SetPushConstant(constants);
			cmd->SetConstantBuffer(sceneData.camera.uniforms, CAMERA_UNIFORMS_BINDING_SLOT);
			cmd->DispatchIndirect(passData.dispatchArgs);
		});

		reflectionTarget = resolveData.radianceOutput;
	}
	else
	{
		history.ReleaseDenoiserTextures(mDevice, graph.GetAllocator());
	}

	RayTracedReflectionData output;
	output.reflectionTarget = reflectionTarget;
	blackboard.Add(output);

	history.frameIndex++;
	history.firstFrame = false;
}

void RayTracedReflectionRenderer::SetSettings(const RayTracedReflectionSettings& settings)
{
	if (memcmp(&mSettings, &settings, sizeof(RayTracedReflectionSettings)) != 0)
	{
		mSettings = settings;
		++mHistoryVersion;
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
