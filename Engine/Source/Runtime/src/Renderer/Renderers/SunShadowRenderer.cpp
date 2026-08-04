#include "gpch.h"
#include "SunShadowRenderer.h"
#include "DepthPrepass.h"
#include "GBufferResolveRenderer.h"

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
	auto renderSystem = Globals::Engine->GetSubsystem<RenderSystem>();
	auto rayTracingScene = renderSystem->GetRayTracingScene();
	mHitGroupTable = HitGroupTable(rayTracingScene);
}

void SunShadowRenderer::OnCreate(const RenderContext& context)
{
	mDevice    = context.device;
	mAllocator = context.allocator;

	ComputePipelineStateDescriptor classificationDesc;
	classificationDesc.entryPoint = "rayTracedSunShadowClassification";
	mClassificationPipeline = context.device->CreateComputePipeline(classificationDesc);

	ComputePipelineStateDescriptor prepareDispatchArgsDesc;
	prepareDispatchArgsDesc.entryPoint = "prepareShadowRayDispatchArgs";
	mPrepareDispatchArgsPipeline = context.device->CreateComputePipeline(prepareDispatchArgsDesc);

	ComputePipelineStateDescriptor tileClassDesc;
	tileClassDesc.entryPoint = "shadowDenoiserTileClassification";
	mTileClassificationPipeline = context.device->CreateComputePipeline(tileClassDesc);

	ComputePipelineStateDescriptor filterDesc;
	filterDesc.entryPoint = "shadowDenoiserFilter";
	mFilterPipeline = context.device->CreateComputePipeline(filterDesc);

	ComputePipelineStateDescriptor resolveDesc;
	resolveDesc.entryPoint = "rayTracedSunShadowResolve";
	mResolvePipeline = context.device->CreateComputePipeline(resolveDesc);

}

void SunShadowRenderer::OnDestroy(const RenderContext& context)
{
	ReleaseDenoiserTextures();
}

void SunShadowRenderer::ReleaseDenoiserTextures()
{
	for (auto& moments : mMoments)
	{
		if (moments.IsValid())
		{
			mDevice->Dispose(mAllocator, moments, BarrierStage::None);
		}
	}

	if (mShadowHistory.IsValid())
	{
		mDevice->Dispose(mAllocator, mShadowHistory, BarrierStage::None);
	}

	mDenoiserSize = Size::zero;
}

void SunShadowRenderer::CreateDenoiserTextures(const Size& size)
{
	if (mDenoiserSize == size)
	{
		return;
	}
	ReleaseDenoiserTextures();

	RenderTextureDescriptor momentsDesc;
	momentsDesc.dimension = TextureDimension::Texture2D;
	momentsDesc.format    = TextureFormat::R11G11B10_SFloat;
	momentsDesc.size      = size;
	momentsDesc.name      = "SunShadowRenderer::ShadowDenoiser::Moments 0";
	mMoments[0] = mDevice->CreateTexture(mAllocator, momentsDesc);
	momentsDesc.name      = "SunShadowRenderer::ShadowDenoiser::Moments 1";
	mMoments[1] = mDevice->CreateTexture(mAllocator, momentsDesc);

	RenderTextureDescriptor historyDesc;
	historyDesc.dimension = TextureDimension::Texture2D;
	historyDesc.format    = TextureFormat::R16G16_SFloat;
	historyDesc.size      = size;
	historyDesc.name      = "SunShadowRenderer::ShadowDenoiser::ShadowHistory";
	mShadowHistory = mDevice->CreateTexture(mAllocator, historyDesc);

	mDenoiserSize = size;
	mFirstFrame = true;
}

void SunShadowRenderer::AddRenderPasses(RenderGraph& graph, RenderGraphBlackboard& blackboard)
{
	if (mSettings.enable == false || mDevice->GetFeatures().raytracing == false)
	{
		ReleaseDenoiserTextures();
		return;
	}

	const auto& sceneData             = blackboard.Get<SceneRenderingData>();
	const auto& depthPrepassData      = blackboard.Get<DepthPrepassData>();
	const auto& gBufferData			  = blackboard.Get<GBufferData>();
	const auto& sceneTargetDescriptor = graph.GetDescriptor(sceneData.sceneTarget);

	const uint32_t width  = (uint32_t)sceneTargetDescriptor.size.width;
	const uint32_t height = (uint32_t)sceneTargetDescriptor.size.height;

	const uint32_t numClassifierTilesX = Math::DivideRoundingUp(width, SHADOW_TILE_WIDTH);
	const uint32_t numClassifierTilesY = Math::DivideRoundingUp(height, SHADOW_TILE_HEIGHT);
	const uint32_t numTiles = numClassifierTilesX * numClassifierTilesY;

	const uint32_t numDenoiserTilesX = Math::DivideRoundingUp(width, 8u);
	const uint32_t numDenoiserTilesY = Math::DivideRoundingUp(height, 8u);

	if (mPipelineDirty)
	{
		RayTracingPipelineStateDescriptor pipelineState;
		pipelineState.rayGenerationEntry = "rayTracedSunShadowRayGen";
		pipelineState.missEntries        = { "rayTracedSunShadowMiss" };
		pipelineState.maxRecursionDepth  = 1;
		pipelineState.maxPayloadSize     = sizeof(ShadowPayload);
		pipelineState.maxAttributeSize   = sizeof(float2);
		pipelineState.hitGroups          = mHitGroupTable.GetDescriptors();

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
		mFirstFrame = true;
	}
	if (mSettings.denoise)
	{
		CreateDenoiserTextures(sceneTargetDescriptor.size);
	}

	// ----------------------------------------------------------------
	// Pass 0 — Ray classification
	// ----------------------------------------------------------------
	struct ClassificationPassData
	{
		TextureHandle depth;
		TextureHandle normalTexture;
		BufferHandle  tileBuffer;
		BufferHandle  tileCount;
	};

	auto& classificationData = graph.AddComputePass<ClassificationPassData>("SunShadowRenderer::Classification",
	[&](RenderGraphBuilder& builder, ClassificationPassData& passData)
	{
		BufferDescriptor tileDesc;
		tileDesc.name = "Shadow Classification Tiles";
		tileDesc.size = numTiles * sizeof(uint32_t) * 4;
		passData.tileBuffer = builder.WriteBuffer(builder.CreateBuffer(tileDesc));

		BufferDescriptor tileCountDesc;
		tileCountDesc.name = "Shadow Classification Tile Count";
		tileCountDesc.size = sizeof(uint32_t);
		passData.tileCount = builder.WriteBuffer(builder.CreateBuffer(tileCountDesc));

		passData.depth         = builder.ReadTexture(depthPrepassData.depthTarget);
		passData.normalTexture = builder.ReadTexture(gBufferData.geometryNormalTarget);
	},
	[this, &sceneData, numClassifierTilesX, numClassifierTilesY](const CommandBuffer* cmd, const ClassificationPassData& passData)
	{
		cmd->ClearBuffer(passData.tileCount);

		Buffer tileCount = passData.tileCount;
		BarrierGroup clearBarrier;
		clearBarrier.bufferBarriers.push_back({
			.resource  = tileCount.GetHandle(),
			.srcStage  = BarrierStage::ClearUnorderedAccess,
			.dstStage  = BarrierStage::ComputeShading,
			.srcAccess = BarrierAccess::UnorderedAccess,
			.dstAccess = BarrierAccess::UnorderedAccess,
		});
		cmd->Barrier(clearBarrier);

		RayTracedSunShadowClassificationConstants constants = {};
		constants.depthTexture    = passData.depth;
		constants.normalTexture   = passData.normalTexture;
		constants.tileBuffer      = passData.tileBuffer;
		constants.tileCountBuffer = passData.tileCount;

		cmd->BindComputePipeline(mClassificationPipeline);
		cmd->SetPushConstant(constants);
		cmd->SetConstantBuffer(sceneData.camera.uniforms, CAMERA_UNIFORMS_BINDING_SLOT);
		cmd->SetConstantBuffer(sceneData.atmosphere.uniforms, SKY_ATMOSPHERE_COMMON_UNIFORMS_BINDING_SLOT);
		cmd->Dispatch(numClassifierTilesX, numClassifierTilesY, 1);
	});

	// ----------------------------------------------------------------
	// Pass 1 — Prepare indirect ray dispatch arguments
	// ----------------------------------------------------------------
	struct PrepareDispatchArgsPassData
	{
		BufferHandle tileCount;
		BufferHandle dispatchArgs;
	};

	auto& prepareArgsData = graph.AddComputePass<PrepareDispatchArgsPassData>("SunShadowRenderer::PrepareDispatchArgs",
	[&](RenderGraphBuilder& builder, PrepareDispatchArgsPassData& passData)
	{
		BufferDescriptor dispatchArgsDesc;
		dispatchArgsDesc.name = "Shadow Ray Dispatch Args";
		dispatchArgsDesc.size = sizeof(DispatchIndirectArguments);
		dispatchArgsDesc.usage = BufferUsage::IndirectArgument;
		passData.dispatchArgs = builder.WriteBuffer(builder.CreateBuffer(dispatchArgsDesc));

		passData.tileCount = builder.ReadBuffer(classificationData.tileCount);
	},
	[this](const CommandBuffer* cmd, const PrepareDispatchArgsPassData& passData)
	{
		PrepareShadowRayDispatchArgsConstants constants = {};
		constants.tileCountBuffer    = passData.tileCount;
		constants.dispatchArgsBuffer = passData.dispatchArgs;

		cmd->BindComputePipeline(mPrepareDispatchArgsPipeline);
		cmd->SetPushConstant(constants);
		cmd->Dispatch(1u, 1u, 1u);
	});

	// ----------------------------------------------------------------
	// Pass 2 — Ray-traced shadow mask
	// ----------------------------------------------------------------
	struct RayTracingPassData
	{
		BufferHandle  shadowMask;
		BufferHandle  tileBuffer;
		BufferHandle  tileCount;
		BufferHandle  dispatchArgs;
		TextureHandle depth;
		TextureHandle normalTexture;
	};

	auto& rayTracingData = graph.AddComputePass<RayTracingPassData>("SunShadowRenderer::RayTracing",
	[&](RenderGraphBuilder& builder, RayTracingPassData& passData)
	{
		BufferDescriptor hitMaskDesc;
		hitMaskDesc.name	= "Shadow HitMask";
		hitMaskDesc.size	= numTiles * sizeof(uint32_t);
		passData.shadowMask    = builder.WriteBuffer(builder.CreateBuffer(hitMaskDesc));
		passData.tileBuffer    = builder.ReadBuffer(classificationData.tileBuffer);
		passData.tileCount     = builder.ReadBuffer(classificationData.tileCount);
		passData.dispatchArgs  = builder.ReadBuffer(prepareArgsData.dispatchArgs);
		passData.depth         = builder.ReadTexture(depthPrepassData.depthTarget);
		passData.normalTexture = builder.ReadTexture(gBufferData.geometryNormalTarget);
	},
	[this, &sceneData](const CommandBuffer* cmd, const RayTracingPassData& passData)
	{
		if (not mRayTracedShadowPipeline.IsValid())
		{
			return;
		}

		cmd->ClearBuffer(passData.shadowMask);

		Buffer shadowMask = passData.shadowMask;
		BarrierGroup clearBarrier;
		clearBarrier.bufferBarriers.push_back({
			.resource  = shadowMask.GetHandle(),
			.srcStage  = BarrierStage::ClearUnorderedAccess,
			.dstStage  = BarrierStage::AllShading,
			.srcAccess = BarrierAccess::UnorderedAccess,
			.dstAccess = BarrierAccess::UnorderedAccess,
		});
		cmd->Barrier(clearBarrier);

		PathTracerConstants pathTraceConstants = {};
		pathTraceConstants.instanceBuffer        = sceneData.sceneProxy->GetGlobalInstanceBuffer().GetResourceView();
		pathTraceConstants.accelerationStructure = sceneData.accelerationStructure;
		pathTraceConstants.colorTarget           = passData.shadowMask;
		pathTraceConstants.frameIndex            = mFrameIndex;
		pathTraceConstants.sceneTarget           = InvalidResourceIndex;
		pathTraceConstants.ggxEssTexture         = InvalidResourceIndex;
		pathTraceConstants.ggxEAvgTexture        = InvalidResourceIndex;
		pathTraceConstants.maxRayRecursionDepth  = 1;
		pathTraceConstants.samplesPerPixel       = 1;

		RayTracedSunShadowConstants constants = {};
		constants.depthTexture    = passData.depth;
		constants.normalTexture   = passData.normalTexture;
		constants.tileBuffer      = passData.tileBuffer;
		constants.tileCountBuffer = passData.tileCount;
		constants.maxRayDistance  = mSettings.maxRayDistance > 0.0f ? mSettings.maxRayDistance : sceneData.camera.uniforms.farPlane;

		cmd->BindRayTracingPipeline(mRayTracedShadowPipeline);
		cmd->SetPushConstant(constants);
		cmd->SetConstantBuffer(pathTraceConstants, PATH_TRACER_CONSTANTS_BINDING_SLOT);
		cmd->SetConstantBuffer(sceneData.camera.uniforms, CAMERA_UNIFORMS_BINDING_SLOT);
		cmd->SetConstantBuffer(sceneData.atmosphere.uniforms, SKY_ATMOSPHERE_COMMON_UNIFORMS_BINDING_SLOT);
		cmd->DispatchRaysIndirect(passData.dispatchArgs);
	});

	TextureHandle shadowMask;
	if (mSettings.denoise)
	{
		// ----------------------------------------------------------------
		// Import persistent denoiser textures + first frame history clear
		// ----------------------------------------------------------------
		const uint32_t prevIndex = mFrameIndex & 1u;
		const uint32_t currIndex = prevIndex ^ 1u;

		ImportResourceParams importParams;
		importParams.clearOnFirstUse = mFirstFrame;

		TextureHandle previousMoments     = graph.ImportTexture(mMoments[prevIndex], importParams);
		TextureHandle currentMoments      = graph.ImportTexture(mMoments[currIndex], importParams);
		TextureHandle historyShadow       = graph.ImportTexture(mShadowHistory, importParams);

		if (mFirstFrame)
		{
			struct ClearHistoryPassData
			{
			};

			graph.AddRenderPass<ClearHistoryPassData>("SunShadowRenderer::ClearHistory",
			[&](RenderGraphBuilder& builder, ClearHistoryPassData& passData)
			{
				previousMoments     = builder.UseColorBuffer(previousMoments);
				currentMoments      = builder.UseColorBuffer(currentMoments);
				historyShadow       = builder.UseColorBuffer(historyShadow);
			},
			[](const CommandBuffer* cmd, const ClearHistoryPassData& passData)
			{
				// Attachments are cleared by the load action on render pass begin
			});
		}

		// ----------------------------------------------------------------
		// Pass 3 — Tile classification
		// ----------------------------------------------------------------
		struct TileClassificationPassData
		{
			BufferHandle  shadowMask;
			TextureHandle depth;
			TextureHandle velocity;
			TextureHandle normalTexture;
			TextureHandle previousDepth;
			TextureHandle previousMoments;
			TextureHandle historyShadow;
			BufferHandle  tileMetadata;
			TextureHandle currentMoments;
			TextureHandle reprojectionResults;
			uint32_t      isFirstFrame;
		};

		auto& tileClassData = graph.AddComputePass<TileClassificationPassData>("SunShadowRenderer::TileClassification",
		[&](RenderGraphBuilder& builder, TileClassificationPassData& passData)
		{
			RenderTextureDescriptor reprojectionDesc;
			reprojectionDesc.name   = "Shadow Reprojection Results";
			reprojectionDesc.size   = sceneTargetDescriptor.size;
			reprojectionDesc.format = TextureFormat::R16G16_SFloat;

			passData.shadowMask				= builder.ReadBuffer(rayTracingData.shadowMask);
			passData.depth					= builder.ReadTexture(depthPrepassData.depthTarget);
			passData.velocity				= builder.ReadTexture(gBufferData.motionVectorTarget);
			passData.normalTexture			= builder.ReadTexture(gBufferData.shadingNormalTarget);
			passData.previousDepth			= builder.ReadTexture(depthPrepassData.previousDepth);
			passData.previousMoments		= builder.ReadTexture(previousMoments);
			passData.currentMoments			= builder.WriteTexture(currentMoments);
			passData.historyShadow			= builder.ReadTexture(historyShadow);
			passData.reprojectionResults	= builder.WriteTexture(builder.CreateTexture(reprojectionDesc));
			passData.isFirstFrame			= mFirstFrame ? 1 : 0;

			BufferDescriptor tileMetaDesc;
			tileMetaDesc.name = "TileMetadata";
			tileMetaDesc.size = numDenoiserTilesX * numDenoiserTilesY * sizeof(uint32_t);
			passData.tileMetadata = builder.WriteBuffer(builder.CreateBuffer(tileMetaDesc));
		
		},
		[this, &sceneData, numDenoiserTilesX, numDenoiserTilesY](const CommandBuffer* cmd, const TileClassificationPassData& passData)
		{
			ShadowDenoiserTileClassificationConstants constants = {};
			constants.reprojectionMatrix  = sceneData.camera.uniforms.projectionMatrix * (sceneData.camera.uniforms.prevViewMatrix * sceneData.camera.uniforms.invViewProjectionMatrix);
			constants.hitMaskResults      = passData.shadowMask;
			constants.depth               = passData.depth;
			constants.velocity            = passData.velocity;
			constants.normalTexture       = passData.normalTexture;
			constants.previousDepth       = passData.previousDepth;
			constants.previousMoments     = passData.previousMoments;
			constants.historyShadow       = passData.historyShadow;
			constants.tileMetadata        = passData.tileMetadata;
			constants.currentMoments      = passData.currentMoments;
			constants.reprojectionResults = passData.reprojectionResults;
			constants.isFirstFrame        = passData.isFirstFrame;

			cmd->BindComputePipeline(mTileClassificationPipeline);
			cmd->SetPushConstant(constants);
			cmd->SetConstantBuffer(sceneData.camera.uniforms, CAMERA_UNIFORMS_BINDING_SLOT);
			cmd->Dispatch(numDenoiserTilesX, numDenoiserTilesY, 1u);
		});

		// ----------------------------------------------------------------
		// Pass 4a — Filter pass 0
		// ----------------------------------------------------------------
		struct Filter0PassData
		{
			TextureHandle depth;
			TextureHandle normalTexture;
			BufferHandle  tileMetadata;
			TextureHandle filterInput;
			TextureHandle history;
		};

		auto& filter0Data = graph.AddComputePass<Filter0PassData>("SunShadowRenderer::Filter Pass 0",
		[&](RenderGraphBuilder& builder, Filter0PassData& passData)
		{
			passData.depth			= builder.ReadTexture(depthPrepassData.depthTarget);
			passData.normalTexture	= builder.ReadTexture(gBufferData.shadingNormalTarget);
			passData.tileMetadata	= builder.ReadBuffer(tileClassData.tileMetadata);
			passData.filterInput	= builder.ReadTexture(tileClassData.reprojectionResults);
			passData.history		= builder.WriteTexture(tileClassData.historyShadow);
		},
		[this, &sceneData, numDenoiserTilesX, numDenoiserTilesY](const CommandBuffer* cmd, const Filter0PassData& passData)
		{
			ShadowDenoiserFilterConstants constants = {};
			constants.depth               = passData.depth;
			constants.normalTexture       = passData.normalTexture;
			constants.tileMetadata        = passData.tileMetadata;
			constants.filterInput         = passData.filterInput;
			constants.history             = passData.history;
			constants.passIndex           = 0u;

			cmd->BindComputePipeline(mFilterPipeline);
			cmd->SetPushConstant(constants);
			cmd->SetConstantBuffer(sceneData.camera.uniforms, CAMERA_UNIFORMS_BINDING_SLOT);
			cmd->Dispatch(numDenoiserTilesX, numDenoiserTilesY, 1u);
		});

		// ----------------------------------------------------------------
		// Pass 4b — Filter pass 1
		// ----------------------------------------------------------------
		struct Filter1PassData
		{
			TextureHandle depth;
			TextureHandle normalTexture;
			BufferHandle  tileMetadata;
			TextureHandle filterInput;
			TextureHandle history;
		};

		auto& filter1Data = graph.AddComputePass<Filter1PassData>("SunShadowRenderer::Filter Pass 1",
		[&](RenderGraphBuilder& builder, Filter1PassData& passData)
		{
			passData.depth        = builder.ReadTexture(depthPrepassData.depthTarget);
			passData.normalTexture = builder.ReadTexture(gBufferData.shadingNormalTarget);
			passData.tileMetadata = builder.ReadBuffer(filter0Data.tileMetadata);
			passData.filterInput  = builder.ReadTexture(filter0Data.history);
			passData.history      = builder.WriteTexture(filter0Data.filterInput);
		},
		[this, &sceneData, numDenoiserTilesX, numDenoiserTilesY](const CommandBuffer* cmd, const Filter1PassData& passData)
		{
			ShadowDenoiserFilterConstants constants = {};
			constants.depth               = passData.depth;
			constants.normalTexture       = passData.normalTexture;
			constants.tileMetadata        = passData.tileMetadata;
			constants.filterInput         = passData.filterInput;
			constants.history             = passData.history;
			constants.passIndex           = 1u;

			cmd->BindComputePipeline(mFilterPipeline);
			cmd->SetPushConstant(constants);
			cmd->SetConstantBuffer(sceneData.camera.uniforms, CAMERA_UNIFORMS_BINDING_SLOT);
			cmd->Dispatch(numDenoiserTilesX, numDenoiserTilesY, 1u);
		});

		// ----------------------------------------------------------------
		// Pass 4c — Filter pass 2
		// ----------------------------------------------------------------
		struct Filter2PassData
		{
			TextureHandle depth;
			TextureHandle normalTexture;
			BufferHandle tileMetadata;
			TextureHandle filterInput;
			TextureHandle shadowMaskOutput;
		};

		auto& filter2Data = graph.AddComputePass<Filter2PassData>("SunShadowRenderer::Filter Pass 2",
		[&](RenderGraphBuilder& builder, Filter2PassData& passData)
		{
			passData.depth        = builder.ReadTexture(depthPrepassData.depthTarget);
			passData.normalTexture = builder.ReadTexture(gBufferData.shadingNormalTarget);
			passData.tileMetadata = builder.ReadBuffer(filter1Data.tileMetadata);
			passData.filterInput  = builder.ReadTexture(filter1Data.history);

			RenderTextureDescriptor shadowMaskDesc;
			shadowMaskDesc.name		= "ShadowMask";
			shadowMaskDesc.size		= sceneTargetDescriptor.size;
			shadowMaskDesc.format	= TextureFormat::R8_UNorm;
			passData.shadowMaskOutput = builder.WriteTexture(builder.CreateTexture(shadowMaskDesc));
		},
		[this, &sceneData, numDenoiserTilesX, numDenoiserTilesY](const CommandBuffer* cmd, const Filter2PassData& passData)
		{
			ShadowDenoiserFilterConstants constants = {};
			constants.depth               = passData.depth;
			constants.normalTexture       = passData.normalTexture;
			constants.tileMetadata        = passData.tileMetadata;
			constants.filterInput         = passData.filterInput;
			constants.shadowMaskOutput    = passData.shadowMaskOutput;
			constants.passIndex           = 2u;

			cmd->BindComputePipeline(mFilterPipeline);
			cmd->SetPushConstant(constants);
			cmd->SetConstantBuffer(sceneData.camera.uniforms, CAMERA_UNIFORMS_BINDING_SLOT);
			cmd->Dispatch(numDenoiserTilesX, numDenoiserTilesY, 1u);
		});

		shadowMask = filter2Data.shadowMaskOutput;
	}
	else
	{
		// ----------------------------------------------------------------
		// Pass 3 — Resolve raw shadow mask
		// ----------------------------------------------------------------
		struct ResolvePassData
		{
			BufferHandle  shadowMask;
			TextureHandle shadowMaskOutput;
		};

		auto& resolveData = graph.AddComputePass<ResolvePassData>("SunShadowRenderer::Resolve",
		[&](RenderGraphBuilder& builder, ResolvePassData& passData)
		{
			RenderTextureDescriptor shadowMaskDesc;
			shadowMaskDesc.name   = "ShadowMask";
			shadowMaskDesc.size   = sceneTargetDescriptor.size;
			shadowMaskDesc.format = TextureFormat::R8_UNorm;

			passData.shadowMask       = builder.ReadBuffer(rayTracingData.shadowMask);
			passData.shadowMaskOutput = builder.WriteTexture(builder.CreateTexture(shadowMaskDesc));
		},
		[this, &sceneData, numDenoiserTilesX, numDenoiserTilesY](const CommandBuffer* cmd, const ResolvePassData& passData)
		{
			RayTracedSunShadowResolveConstants constants = {};
			constants.hitMaskResults   = passData.shadowMask;
			constants.shadowMaskOutput = passData.shadowMaskOutput;

			cmd->BindComputePipeline(mResolvePipeline);
			cmd->SetPushConstant(constants);
			cmd->SetConstantBuffer(sceneData.camera.uniforms, CAMERA_UNIFORMS_BINDING_SLOT);
			cmd->Dispatch(numDenoiserTilesX, numDenoiserTilesY, 1u);
		});

		shadowMask = resolveData.shadowMaskOutput;
	}

	SunShadowData output;
	output.shadowMask = shadowMask;
	blackboard.Add(output);

	mFrameIndex++;
	mFirstFrame = false;
}

void SunShadowRenderer::SetSettings(const ShadowSettings& settings)
{
	if (memcmp(&mSettings, &settings, sizeof(ShadowSettings)) != 0)
	{
		if (mSettings.denoise != settings.denoise)
		{
			ReleaseDenoiserTextures();
		}

		mSettings = settings;
	}
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
