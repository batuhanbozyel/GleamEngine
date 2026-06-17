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
	context.device->Dispose(context.allocator, mMoments[0], BarrierStage::None);
	context.device->Dispose(context.allocator, mMoments[1], BarrierStage::None);
	context.device->Dispose(context.allocator, mScratch[0],	BarrierStage::None);
	context.device->Dispose(context.allocator, mScratch[1], BarrierStage::None);
	context.device->Dispose(context.allocator, mPreviousDepth, BarrierStage::None);
}

void SunShadowRenderer::CreateDenoiserTextures(const Size& size)
{
	if (mDenoiserSize == size)
	{
		return;
	}

	if (mMoments[0].IsValid())
	{
		mDevice->Dispose(mAllocator, mMoments[0], BarrierStage::None);
	}
	if (mMoments[1].IsValid())
	{
		mDevice->Dispose(mAllocator, mMoments[1], BarrierStage::None);
	}
	if (mScratch[0].IsValid())
	{
		mDevice->Dispose(mAllocator, mScratch[0], BarrierStage::None);
	}
	if (mScratch[1].IsValid())
	{
		mDevice->Dispose(mAllocator, mScratch[1], BarrierStage::None);
	}
	if (mPreviousDepth.IsValid())
	{
		mDevice->Dispose(mAllocator, mPreviousDepth, BarrierStage::None);
	}

	TextureDescriptor momentsDesc;
	momentsDesc.dimension = TextureDimension::Texture2D;
	momentsDesc.format    = TextureFormat::R11G11B10_SFloat;
	momentsDesc.usage     = TextureUsage_Storage | TextureUsage_Sampled;
	momentsDesc.size      = size;
	momentsDesc.name      = "SunShadowRenderer::ShadowDenoiser::Moments 0";
	mMoments[0] = mDevice->CreateTexture(mAllocator, momentsDesc);
	momentsDesc.name      = "SunShadowRenderer::ShadowDenoiser::Moments 1";
	mMoments[1] = mDevice->CreateTexture(mAllocator, momentsDesc);

	TextureDescriptor scratchDesc;
	scratchDesc.dimension = TextureDimension::Texture2D;
	scratchDesc.format    = TextureFormat::R16G16_SFloat;
	scratchDesc.usage     = TextureUsage_Storage | TextureUsage_Sampled;
	scratchDesc.size      = size;
	scratchDesc.name      = "SunShadowRenderer::ShadowDenoiser::Scratch 0";
	mScratch[0] = mDevice->CreateTexture(mAllocator, scratchDesc);
	scratchDesc.name      = "SunShadowRenderer::ShadowDenoiser::Scratch 1";
	mScratch[1] = mDevice->CreateTexture(mAllocator, scratchDesc);

	TextureDescriptor depthDesc;
	depthDesc.name      = "SunShadowRenderer::ShadowDenoiser::PreviousDepth";
	depthDesc.dimension = TextureDimension::Texture2D;
	depthDesc.format    = TextureFormat::R32_SFloat;
	depthDesc.usage     = TextureUsage_Storage | TextureUsage_Sampled;
	depthDesc.size      = size;
	mPreviousDepth = mDevice->CreateTexture(mAllocator, depthDesc);

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
	const uint32_t numTiles = Math::DivideRoundingUp(width, SHADOW_TILE_WIDTH) * Math::DivideRoundingUp(height, SHADOW_TILE_HEIGHT);

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
		mFirstFrame = true;
	}
	CreateDenoiserTextures(sceneTargetDescriptor.size);

	// ----------------------------------------------------------------
	// Pass 1 — Ray-traced shadow mask
	// ----------------------------------------------------------------
	struct RayTracingPassData
	{
		BufferHandle  shadowMask;
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
		passData.depth         = builder.ReadTexture(depthPrepassData.depthTarget);
		passData.normalTexture = builder.ReadTexture(depthPrepassData.normalTarget);
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
		constants.depthTexture  = passData.depth;
		constants.normalTexture = passData.normalTexture;

		cmd->BindRayTracingPipeline(mRayTracedShadowPipeline);
		cmd->SetPushConstant(constants);
		cmd->SetConstantBuffer(pathTraceConstants, PATH_TRACER_CONSTANTS_BINDING_SLOT);
		cmd->SetConstantBuffer(sceneData.camera.uniforms, CAMERA_UNIFORMS_BINDING_SLOT);
		cmd->SetConstantBuffer(sceneData.atmosphere.uniforms, SKY_ATMOSPHERE_COMMON_UNIFORMS_BINDING_SLOT);
		cmd->DispatchRays((uint32_t)sceneData.camera.uniforms.resolution.x,
		                  (uint32_t)sceneData.camera.uniforms.resolution.y, 1u);
	});

	// ----------------------------------------------------------------
	// Pass 2 — Tile classification
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
		const uint32_t prevIndex = mFrameIndex & 1u;
		const uint32_t currIndex = prevIndex ^ 1u;

		ImportResourceParams importParams;
		importParams.clearOnFirstUse = mFirstFrame;

		passData.shadowMask				= builder.ReadBuffer(rayTracingData.shadowMask);
		passData.depth					= builder.ReadTexture(depthPrepassData.depthTarget);
		passData.velocity				= builder.ReadTexture(depthPrepassData.motionVectorTarget);
		passData.normalTexture			= builder.ReadTexture(depthPrepassData.normalTarget);
		passData.previousDepth			= builder.ReadTexture(graph.ImportTexture(mPreviousDepth, importParams));
		passData.previousMoments		= builder.ReadTexture(graph.ImportTexture(mMoments[prevIndex], importParams));
		passData.currentMoments			= builder.WriteTexture(graph.ImportTexture(mMoments[currIndex], importParams));
		passData.historyShadow			= builder.ReadTexture(graph.ImportTexture(mScratch[1], importParams));
		passData.reprojectionResults	= builder.WriteTexture(graph.ImportTexture(mScratch[0], importParams));
		passData.isFirstFrame			= mFirstFrame ? 1 : 0;

		BufferDescriptor tileMetaDesc;
		tileMetaDesc.name = "TileMetadata";
		tileMetaDesc.size = numTiles * sizeof(uint32_t);
		passData.tileMetadata = builder.WriteBuffer(builder.CreateBuffer(tileMetaDesc));
		
	},
	[this, &sceneData, width, height](const CommandBuffer* cmd, const TileClassificationPassData& passData)
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
		cmd->Dispatch(Math::DivideRoundingUp(width, 8u),
		              Math::DivideRoundingUp(height, 8u), 1u);
	});

	// ----------------------------------------------------------------
	// Pass 3a — Filter pass 0
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
		passData.normalTexture	= builder.ReadTexture(depthPrepassData.normalTarget);
		passData.tileMetadata	= builder.ReadBuffer(tileClassData.tileMetadata);
		passData.filterInput	= builder.ReadTexture(tileClassData.reprojectionResults);
		passData.history		= builder.WriteTexture(tileClassData.historyShadow);
	},
	[this, &sceneData, width, height](const CommandBuffer* cmd, const Filter0PassData& passData)
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
		cmd->Dispatch(Math::DivideRoundingUp(width, 8u),
		              Math::DivideRoundingUp(height, 8u), 1u);
	});

	// ----------------------------------------------------------------
	// Pass 3b — Filter pass 1
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
		passData.normalTexture = builder.ReadTexture(depthPrepassData.normalTarget);
		passData.tileMetadata = builder.ReadBuffer(filter0Data.tileMetadata);
		passData.filterInput  = builder.ReadTexture(filter0Data.history);
		passData.history      = builder.WriteTexture(filter0Data.filterInput);
	},
	[this, &sceneData, width, height](const CommandBuffer* cmd, const Filter1PassData& passData)
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
		cmd->Dispatch(Math::DivideRoundingUp(width, 8u),
		              Math::DivideRoundingUp(height, 8u), 1u);
	});

	// ----------------------------------------------------------------
	// Pass 3c — Filter pass 2
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
		passData.normalTexture = builder.ReadTexture(depthPrepassData.normalTarget);
		passData.tileMetadata = builder.ReadBuffer(filter1Data.tileMetadata);
		passData.filterInput  = builder.ReadTexture(filter1Data.history);

		RenderTextureDescriptor shadowMaskDesc;
		shadowMaskDesc.name		= "ShadowMask";
		shadowMaskDesc.size		= sceneTargetDescriptor.size;
		shadowMaskDesc.format	= TextureFormat::R8_UNorm;
		passData.shadowMaskOutput = builder.WriteTexture(builder.CreateTexture(shadowMaskDesc));
	},
	[this, &sceneData, width, height](const CommandBuffer* cmd, const Filter2PassData& passData)
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
		passData.sourceDepth = builder.ReadTexture(filter2Data.depth);
		passData.destDepth   = builder.WriteTexture(tileClassData.previousDepth);
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
