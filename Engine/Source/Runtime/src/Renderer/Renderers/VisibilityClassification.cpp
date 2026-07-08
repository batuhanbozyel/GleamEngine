#include "gpch.h"
#include "VisibilityClassification.h"
#include "DepthPrepass.h"

#include "Core/Engine.h"
#include "Core/Globals.h"

#include "Renderer/CommandBuffer.h"
#include "Renderer/GraphicsDevice.h"

#include "World/Systems/RenderSceneProxy.h"

using namespace Gleam;

void VisibilityClassificationRenderer::OnCreate(const RenderContext& context)
{
	mDevice = context.device;

	ComputePipelineStateDescriptor countDesc;
	countDesc.entryPoint = "visibilityCountShader";
	mCountPipeline = context.device->CreateComputePipeline(countDesc);

	ComputePipelineStateDescriptor allocateDesc;
	allocateDesc.entryPoint = "visibilityAllocateShader";
	mAllocatePipeline = context.device->CreateComputePipeline(allocateDesc);

	ComputePipelineStateDescriptor scatterDesc;
	scatterDesc.entryPoint = "visibilityScatterShader";
	mScatterPipeline = context.device->CreateComputePipeline(scatterDesc);
}

void VisibilityClassificationRenderer::OnDestroy(const RenderContext& context)
{

}

void VisibilityClassificationRenderer::AddRenderPasses(RenderGraph& graph, RenderGraphBlackboard& blackboard)
{
	const auto& sceneData = blackboard.Get<SceneRenderingData>();
	const auto& depthPrepassData = blackboard.Get<DepthPrepassData>();
	const auto& sceneTargetDescriptor = graph.GetDescriptor(sceneData.sceneTarget);
	const uint32_t width = (uint32_t)sceneTargetDescriptor.size.width;
	const uint32_t height = (uint32_t)sceneTargetDescriptor.size.height;

	const uint32_t batchCount = sceneData.sceneProxy->GetBatchCount();
	const uint32_t numBatches = batchCount > 0 ? batchCount : 1u;

	// ----------------------------------------------------------------
	// Pass 0 — Count pixels per material batch
	// ----------------------------------------------------------------
	struct CountPassData
	{
		TextureHandle visibilityBuffer;
		BufferHandle countsBuffer;
	};

	const auto& countData = graph.AddComputePass<CountPassData>("VisibilityClassification::Count",
	[&](RenderGraphBuilder& builder, CountPassData& passData)
	{
		BufferDescriptor countsDesc;
		countsDesc.name = "Visibility Batch Counts";
		countsDesc.size = numBatches * sizeof(uint32_t);
		passData.countsBuffer = builder.WriteBuffer(builder.CreateBuffer(countsDesc));

		passData.visibilityBuffer = builder.ReadTexture(depthPrepassData.visibilityBuffer);
	},
	[this, &sceneData, width, height](const CommandBuffer* cmd, const CountPassData& passData)
	{
		cmd->ClearBuffer(passData.countsBuffer);

		Buffer countsBuffer = passData.countsBuffer;
		BarrierGroup clearBarrier;
		clearBarrier.bufferBarriers.push_back({
			.resource  = countsBuffer.GetHandle(),
			.srcStage  = BarrierStage::ClearUnorderedAccess,
			.dstStage  = BarrierStage::ComputeShading,
			.srcAccess = BarrierAccess::UnorderedAccess,
			.dstAccess = BarrierAccess::UnorderedAccess,
		});
		cmd->Barrier(clearBarrier);

		VisibilityClassifyConstants constants = {};
		constants.visibilityBuffer = passData.visibilityBuffer;
		constants.countsBuffer = passData.countsBuffer;

		cmd->BindComputePipeline(mCountPipeline);
		cmd->SetPushConstant(constants);
		cmd->SetConstantBuffer(sceneData.camera.uniforms, CAMERA_UNIFORMS_BINDING_SLOT);
		cmd->Dispatch(Math::DivideRoundingUp(width, 8u), Math::DivideRoundingUp(height, 8u), 1);
	});

	// ----------------------------------------------------------------
	// Pass 1 — Exclusive scan of counts + indirect dispatch args
	// ----------------------------------------------------------------
	struct AllocatePassData
	{
		BufferHandle countsBuffer;
		BufferHandle offsetsBuffer;
		BufferHandle cursorsBuffer;
		BufferHandle dispatchArgsBuffer;
	};

	const auto& allocateData = graph.AddComputePass<AllocatePassData>("VisibilityClassification::Allocate",
	[&](RenderGraphBuilder& builder, AllocatePassData& passData)
	{
		BufferDescriptor offsetsDesc;
		offsetsDesc.name = "Visibility Batch Offsets";
		offsetsDesc.size = numBatches * sizeof(uint32_t);
		passData.offsetsBuffer = builder.WriteBuffer(builder.CreateBuffer(offsetsDesc));

		BufferDescriptor cursorsDesc;
		cursorsDesc.name = "Visibility Batch Cursors";
		cursorsDesc.size = numBatches * sizeof(uint32_t);
		passData.cursorsBuffer = builder.WriteBuffer(builder.CreateBuffer(cursorsDesc));

		BufferDescriptor dispatchArgsDesc;
		dispatchArgsDesc.name = "Visibility Dispatch Args";
		dispatchArgsDesc.size = numBatches * sizeof(DispatchIndirectArguments);
		passData.dispatchArgsBuffer = builder.WriteBuffer(builder.CreateBuffer(dispatchArgsDesc));

		passData.countsBuffer = builder.ReadBuffer(countData.countsBuffer);
	},
	[this, numBatches](const CommandBuffer* cmd, const AllocatePassData& passData)
	{
		VisibilityAllocateConstants constants = {};
		constants.countsBuffer = passData.countsBuffer;
		constants.offsetsBuffer = passData.offsetsBuffer;
		constants.cursorsBuffer = passData.cursorsBuffer;
		constants.dispatchArgsBuffer = passData.dispatchArgsBuffer;
		constants.numBatches = numBatches;

		cmd->BindComputePipeline(mAllocatePipeline);
		cmd->SetPushConstant(constants);
		cmd->Dispatch(1, 1, 1);
	});

	// ----------------------------------------------------------------
	// Pass 2 — Scatter pixels into per-batch lists
	// ----------------------------------------------------------------
	struct ScatterPassData
	{
		TextureHandle visibilityBuffer;
		BufferHandle cursorsBuffer;
		BufferHandle pixelListBuffer;
	};

	const auto& scatterData = graph.AddComputePass<ScatterPassData>("VisibilityClassification::Scatter",
	[&](RenderGraphBuilder& builder, ScatterPassData& passData)
	{
		BufferDescriptor pixelListDesc;
		pixelListDesc.name = "Visibility Pixel List";
		pixelListDesc.size = width * height * sizeof(uint32_t);
		passData.pixelListBuffer = builder.WriteBuffer(builder.CreateBuffer(pixelListDesc));

		passData.cursorsBuffer = builder.WriteBuffer(allocateData.cursorsBuffer);
		passData.visibilityBuffer = builder.ReadTexture(depthPrepassData.visibilityBuffer);
	},
	[this, &sceneData, width, height](const CommandBuffer* cmd, const ScatterPassData& passData)
	{
		VisibilityClassifyConstants constants = {};
		constants.visibilityBuffer = passData.visibilityBuffer;
		constants.cursorsBuffer = passData.cursorsBuffer;
		constants.pixelListBuffer = passData.pixelListBuffer;

		cmd->BindComputePipeline(mScatterPipeline);
		cmd->SetPushConstant(constants);
		cmd->SetConstantBuffer(sceneData.camera.uniforms, CAMERA_UNIFORMS_BINDING_SLOT);
		cmd->Dispatch(Math::DivideRoundingUp(width, 8u), Math::DivideRoundingUp(height, 8u), 1);
	});

	VisibilityClassificationData visibilityClassificationData;
	visibilityClassificationData.pixelListBuffer = scatterData.pixelListBuffer;
	visibilityClassificationData.offsetsBuffer = allocateData.offsetsBuffer;
	visibilityClassificationData.countsBuffer = countData.countsBuffer;
	visibilityClassificationData.dispatchArgsBuffer = allocateData.dispatchArgsBuffer;
	blackboard.Add(visibilityClassificationData);
}
