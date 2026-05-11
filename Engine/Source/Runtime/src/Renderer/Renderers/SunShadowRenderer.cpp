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
	mDevice    = context.device;
	mAllocator = context.allocator;

	ComputePipelineStateDescriptor tileClassDesc;
	tileClassDesc.entryPoint = "shadowDenoiserTileClassification";
	mTileClassificationPipeline = context.device->CreateComputePipeline(tileClassDesc);

	ComputePipelineStateDescriptor filterDesc;
	filterDesc.entryPoint = "shadowDenoiserFilter";
	mFilterPipeline = context.device->CreateComputePipeline(filterDesc);

	ComputePipelineStateDescriptor depthCopyDesc;
	depthCopyDesc.entryPoint = "shadowDenoiserDepthCopy";
	mDepthCopyPipeline = context.device->CreateComputePipeline(depthCopyDesc);
}

void SunShadowRenderer::OnDestroy(const RenderContext& context)
{
	context.device->Dispose(context.allocator, mMoments[0],       BarrierStage::None);
	context.device->Dispose(context.allocator, mMoments[1],       BarrierStage::None);
	context.device->Dispose(context.allocator, mHistoryShadow[0], BarrierStage::None);
	context.device->Dispose(context.allocator, mHistoryShadow[1], BarrierStage::None);
	context.device->Dispose(context.allocator, mPreviousDepth,    BarrierStage::None);
	context.device->Dispose(context.allocator, mDenoisedShadow,   BarrierStage::None);
}

void SunShadowRenderer::CreateDenoiserTextures(const Size& size)
{
	if (mDenoiserSize == size)
	{
		return;
	}

	mDevice->Dispose(mAllocator, mMoments[0],       BarrierStage::None);
	mDevice->Dispose(mAllocator, mMoments[1],       BarrierStage::None);
	mDevice->Dispose(mAllocator, mHistoryShadow[0], BarrierStage::None);
	mDevice->Dispose(mAllocator, mHistoryShadow[1], BarrierStage::None);
	mDevice->Dispose(mAllocator, mPreviousDepth,    BarrierStage::None);
	mDevice->Dispose(mAllocator, mDenoisedShadow,   BarrierStage::None);

	TextureDescriptor momentsDesc;
	momentsDesc.dimension = TextureDimension::Texture2D;
	momentsDesc.format    = TextureFormat::R16G16B16A16_SFloat;
	momentsDesc.usage     = TextureUsage_Storage | TextureUsage_Sampled;
	momentsDesc.size      = size;
	momentsDesc.name      = "ShadowDenoiser Moments 0";
	mMoments[0] = mDevice->CreateTexture(mAllocator, momentsDesc);
	momentsDesc.name      = "ShadowDenoiser Moments 1";
	mMoments[1] = mDevice->CreateTexture(mAllocator, momentsDesc);

	TextureDescriptor historyDesc;
	historyDesc.dimension = TextureDimension::Texture2D;
	historyDesc.format    = TextureFormat::R16G16_SFloat;
	historyDesc.usage     = TextureUsage_Storage | TextureUsage_Sampled;
	historyDesc.size      = size;
	historyDesc.name      = "ShadowDenoiser History 0";
	mHistoryShadow[0] = mDevice->CreateTexture(mAllocator, historyDesc);
	historyDesc.name      = "ShadowDenoiser History 1";
	mHistoryShadow[1] = mDevice->CreateTexture(mAllocator, historyDesc);

	TextureDescriptor depthDesc;
	depthDesc.name      = "ShadowDenoiser PreviousDepth";
	depthDesc.dimension = TextureDimension::Texture2D;
	depthDesc.format    = TextureFormat::R32_SFloat;
	depthDesc.usage     = TextureUsage_Storage | TextureUsage_Sampled;
	depthDesc.size      = size;
	mPreviousDepth = mDevice->CreateTexture(mAllocator, depthDesc);

	TextureDescriptor outputDesc;
	outputDesc.name      = "ShadowDenoiser Output";
	outputDesc.dimension = TextureDimension::Texture2D;
	outputDesc.format    = TextureFormat::R8_UNorm;
	outputDesc.usage     = TextureUsage_Storage | TextureUsage_Sampled;
	outputDesc.size      = size;
	mDenoisedShadow = mDevice->CreateTexture(mAllocator, outputDesc);

	mDenoiserSize = size;
	mFirstFrame = true;
}

void SunShadowRenderer::AddRenderPasses(RenderGraph& graph, RenderGraphBlackboard& blackboard)
{
	const auto& sceneData             = blackboard.Get<SceneRenderingData>();
	const auto& depthPrepassData      = blackboard.Get<DepthPrepassData>();
	const auto& sceneTargetDescriptor = graph.GetDescriptor(sceneData.sceneTarget);

	const uint32_t width  = (uint32_t)sceneTargetDescriptor.size.width;
	const uint32_t height = (uint32_t)sceneTargetDescriptor.size.height;

	if (mPipelineDirty)
	{
		RayTracingPipelineStateDescriptor pipelineState;
		pipelineState.rayGenerationEntry = "rayTracedSunShadowRayGen";
		pipelineState.missEntries        = { "rayTracedSunShadowMiss" };
		pipelineState.maxRecursionDepth  = 2;
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
	}
	CreateDenoiserTextures(sceneTargetDescriptor.size);

	const uint32_t prevIndex = mFrameIndex & 1u;
	const uint32_t currIndex = prevIndex ^ 1u;

	auto importedPrevMoments    = graph.ImportTexture(mMoments[prevIndex]);
	auto importedCurrMoments    = graph.ImportTexture(mMoments[currIndex]);
	auto importedPrevHistory    = graph.ImportTexture(mHistoryShadow[prevIndex]);
	auto importedCurrHistory    = graph.ImportTexture(mHistoryShadow[currIndex]);
	auto importedPreviousDepth  = graph.ImportTexture(mPreviousDepth);
	auto importedDenoisedShadow = graph.ImportTexture(mDenoisedShadow);

	// ----------------------------------------------------------------
	// Pass 1 — Ray-traced shadow mask
	// ----------------------------------------------------------------
	struct RayTracingPassData
	{
		TextureHandle shadowMask;
		TextureHandle depth;
	};

	auto& rayTracingData = graph.AddComputePass<RayTracingPassData>("SunShadowRenderer::RayTracing",
	[&](RenderGraphBuilder& builder, RayTracingPassData& passData)
	{
		RenderTextureDescriptor shadowDesc;
		shadowDesc.name   = "SunShadowMask";
		shadowDesc.size   = Size{
			(float)Math::DivideRoundingUp(width,  SHADOW_TILE_WIDTH),
			(float)Math::DivideRoundingUp(height, SHADOW_TILE_HEIGHT)
		};
		shadowDesc.format = TextureFormat::R32_UInt;
		passData.shadowMask = builder.WriteTexture(builder.CreateTexture(shadowDesc));
		passData.depth      = builder.ReadTexture(depthPrepassData.depthTarget);
	},
	[this, &sceneData](const CommandBuffer* cmd, const RayTracingPassData& passData)
	{
		if (not mRayTracedShadowPipeline.IsValid())
		{
			return;
		}

		PathTracerConstants pathTraceConstants = {};
		pathTraceConstants.instanceBuffer        = sceneData.sceneProxy->GetGlobalInstanceBuffer().GetResourceView();
		pathTraceConstants.accelerationStructure = sceneData.accelerationStructure;
		pathTraceConstants.colorTarget           = passData.shadowMask;
		pathTraceConstants.frameIndex            = mFrameIndex;
		pathTraceConstants.sceneTarget           = InvalidResourceIndex;
		pathTraceConstants.ggxEssTexture         = InvalidResourceIndex;
		pathTraceConstants.ggxEAvgTexture        = InvalidResourceIndex;
		pathTraceConstants.maxRayRecursionDepth  = 2;
		pathTraceConstants.samplesPerPixel       = 1;

		RayTracedSunShadowConstants constants = {};
		constants.depthTexture = passData.depth;

		cmd->BindRayTracingPipeline(mRayTracedShadowPipeline);
		cmd->SetPushConstant(constants);
		cmd->SetConstantBuffer(pathTraceConstants, PATH_TRACER_CONSTANTS_BINDING_SLOT);
		cmd->SetConstantBuffer(sceneData.camera.uniforms, CAMERA_UNIFORMS_BINDING_SLOT);
		cmd->SetConstantBuffer(sceneData.atmosphere.uniforms, SKY_ATMOSPHERE_COMMON_UNIFORMS_BINDING_SLOT);
		cmd->DispatchRays((uint32_t)sceneData.camera.uniforms.resolution.x,
		                  (uint32_t)sceneData.camera.uniforms.resolution.y, 1u);
	});

	// ----------------------------------------------------------------
	// Pass 2 — Tile classification + temporal reprojection
	// ----------------------------------------------------------------
	struct TileClassificationPassData
	{
		TextureHandle shadowMask;
		TextureHandle depth;
		TextureHandle velocity;
		TextureHandle previousDepth;
		TextureHandle previousMoments;
		TextureHandle historyShadow;
		TextureHandle tileMetadata;
		TextureHandle currentMoments;
		TextureHandle reprojectionResults;
		int32_t       isFirstFrame;
	};

	auto& tileClassData = graph.AddComputePass<TileClassificationPassData>("SunShadowRenderer::TileClassification",
	[&](RenderGraphBuilder& builder, TileClassificationPassData& passData)
	{
		passData.shadowMask     = builder.ReadTexture(rayTracingData.shadowMask);
		passData.depth          = builder.ReadTexture(depthPrepassData.depthTarget);
		passData.velocity       = builder.ReadTexture(depthPrepassData.motionVectorTarget);
		passData.previousDepth  = builder.ReadTexture(importedPreviousDepth);
		passData.previousMoments = builder.ReadTexture(importedPrevMoments);
		passData.historyShadow  = builder.ReadTexture(importedPrevHistory);
		passData.isFirstFrame   = mFirstFrame ? 1 : 0;

		RenderTextureDescriptor tileMetaDesc;
		tileMetaDesc.name   = "ShadowDenoiser TileMetadata";
		tileMetaDesc.size   = Size{
			(float)(Math::DivideRoundingUp(width, 8u) * Math::DivideRoundingUp(height, 4u)),
			1.0f
		};
		tileMetaDesc.format = TextureFormat::R32_UInt;
		passData.tileMetadata = builder.WriteTexture(builder.CreateTexture(tileMetaDesc));
		passData.currentMoments = builder.WriteTexture(importedCurrMoments);

		RenderTextureDescriptor reproDesc;
		reproDesc.name   = "ShadowDenoiser ReprojectionResults";
		reproDesc.size   = Size{ (float)width, (float)height };
		reproDesc.format = TextureFormat::R16G16_SFloat;
		passData.reprojectionResults = builder.WriteTexture(builder.CreateTexture(reproDesc));
	},
	[this, &sceneData, width, height](const CommandBuffer* cmd, const TileClassificationPassData& passData)
	{
		ShadowDenoiserTileClassificationConstants constants = {};
		constants.hitMaskResults      = passData.shadowMask;
		constants.depth               = passData.depth;
		constants.velocity            = passData.velocity;
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
		cmd->Dispatch(Math::DivideRoundingUp(width, 8u),
		              Math::DivideRoundingUp(height, 8u), 1u);
	});

	// ----------------------------------------------------------------
	// Pass 3a — Filter pass 0: reprojectionResults → filterIntermediate
	// ----------------------------------------------------------------
	struct Filter0PassData
	{
		TextureHandle depth;
		TextureHandle tileMetadata;
		TextureHandle filterInput;
		TextureHandle history;
	};

	auto& filter0Data = graph.AddComputePass<Filter0PassData>("SunShadowRenderer::Filter Pass 0",
	[&](RenderGraphBuilder& builder, Filter0PassData& passData)
	{
		passData.depth        = builder.ReadTexture(depthPrepassData.depthTarget);
		passData.tileMetadata = builder.ReadTexture(tileClassData.tileMetadata);
		passData.filterInput  = builder.ReadTexture(tileClassData.reprojectionResults);

		RenderTextureDescriptor intermediateDesc;
		intermediateDesc.name   = "ShadowDenoiser FilterIntermediate";
		intermediateDesc.size   = Size{ (float)width, (float)height };
		intermediateDesc.format = TextureFormat::R16G16_SFloat;
		passData.history = builder.WriteTexture(builder.CreateTexture(intermediateDesc));
	},
	[this, &sceneData, width, height](const CommandBuffer* cmd, const Filter0PassData& passData)
	{
		ShadowDenoiserFilterConstants constants = {};
		constants.depth               = passData.depth;
		constants.tileMetadata        = passData.tileMetadata;
		constants.filterInput         = passData.filterInput;
		constants.history             = passData.history;
		constants.passIndex           = 0u;
		constants.stepSize            = 1u;

		cmd->BindComputePipeline(mFilterPipeline);
		cmd->SetPushConstant(constants);
		cmd->SetConstantBuffer(sceneData.camera.uniforms, CAMERA_UNIFORMS_BINDING_SLOT);
		cmd->Dispatch(Math::DivideRoundingUp(width, 8u),
		              Math::DivideRoundingUp(height, 8u), 1u);
	});

	// ----------------------------------------------------------------
	// Pass 3b — Filter pass 1: filterIntermediate → historyShadow[curr]
	// ----------------------------------------------------------------
	struct Filter1PassData
	{
		TextureHandle depth;
		TextureHandle tileMetadata;
		TextureHandle filterInput;
		TextureHandle history;
	};

	auto& filter1Data = graph.AddComputePass<Filter1PassData>("SunShadowRenderer::Filter Pass 1",
	[&](RenderGraphBuilder& builder, Filter1PassData& passData)
	{
		passData.depth        = builder.ReadTexture(depthPrepassData.depthTarget);
		passData.tileMetadata = builder.ReadTexture(filter0Data.tileMetadata);
		passData.filterInput  = builder.ReadTexture(filter0Data.history);
		passData.history      = builder.WriteTexture(importedCurrHistory);
	},
	[this, &sceneData, width, height](const CommandBuffer* cmd, const Filter1PassData& passData)
	{
		ShadowDenoiserFilterConstants constants = {};
		constants.depth               = passData.depth;
		constants.tileMetadata        = passData.tileMetadata;
		constants.filterInput         = passData.filterInput;
		constants.history             = passData.history;
		constants.passIndex           = 1u;
		constants.stepSize            = 2u;

		cmd->BindComputePipeline(mFilterPipeline);
		cmd->SetPushConstant(constants);
		cmd->SetConstantBuffer(sceneData.camera.uniforms, CAMERA_UNIFORMS_BINDING_SLOT);
		cmd->Dispatch(Math::DivideRoundingUp(width, 8u),
		              Math::DivideRoundingUp(height, 8u), 1u);
	});

	// ----------------------------------------------------------------
	// Pass 3c — Filter pass 2: historyShadow[curr] → denoisedShadow
	// ----------------------------------------------------------------
	struct Filter2PassData
	{
		TextureHandle depth;
		TextureHandle tileMetadata;
		TextureHandle filterInput;
		TextureHandle shadowMaskOutput;
	};

	auto& filter2Data = graph.AddComputePass<Filter2PassData>("SunShadowRenderer::Filter Pass 2",
	[&](RenderGraphBuilder& builder, Filter2PassData& passData)
	{
		passData.depth        = builder.ReadTexture(depthPrepassData.depthTarget);
		passData.tileMetadata = builder.ReadTexture(filter1Data.tileMetadata);
		passData.filterInput  = builder.ReadTexture(filter1Data.history);
		passData.shadowMaskOutput = builder.WriteTexture(importedDenoisedShadow);
	},
	[this, &sceneData, width, height](const CommandBuffer* cmd, const Filter2PassData& passData)
	{
		ShadowDenoiserFilterConstants constants = {};
		constants.depth               = passData.depth;
		constants.tileMetadata        = passData.tileMetadata;
		constants.filterInput         = passData.filterInput;
		constants.shadowMaskOutput    = passData.shadowMaskOutput;
		constants.passIndex           = 2u;
		constants.stepSize            = 4u;

		cmd->BindComputePipeline(mFilterPipeline);
		cmd->SetPushConstant(constants);
		cmd->SetConstantBuffer(sceneData.camera.uniforms, CAMERA_UNIFORMS_BINDING_SLOT);
		cmd->Dispatch(Math::DivideRoundingUp(width, 8u),
		              Math::DivideRoundingUp(height, 8u), 1u);
	});

	// ----------------------------------------------------------------
	// Pass 4 — Copy current depth to previousDepth for next frame
	// ----------------------------------------------------------------
	struct DepthCopyPassData
	{
		TextureHandle sourceDepth;
		TextureHandle destDepth;
	};

	graph.AddComputePass<DepthCopyPassData>("SunShadowRenderer::DepthCopy",
	[&](RenderGraphBuilder& builder, DepthCopyPassData& passData)
	{
		passData.sourceDepth = builder.ReadTexture(depthPrepassData.depthTarget);
		passData.destDepth   = builder.WriteTexture(importedPreviousDepth);
	},
	[this, width, height](const CommandBuffer* cmd, const DepthCopyPassData& passData)
	{
		ShadowDenoiserDepthCopyConstants constants = {};
		constants.sourceDepth = passData.sourceDepth;
		constants.destDepth   = passData.destDepth;

		cmd->BindComputePipeline(mDepthCopyPipeline);
		cmd->SetPushConstant(constants);
		cmd->Dispatch(Math::DivideRoundingUp(width, 8u),
		              Math::DivideRoundingUp(height, 8u), 1u);
	});

	SunShadowData output;
	output.depthTarget = depthPrepassData.depthTarget;
	output.shadowMask = filter2Data.shadowMaskOutput;
	blackboard.Add(output);

	mFrameIndex++;
	mFirstFrame = false;
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
