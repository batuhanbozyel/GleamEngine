#include "gpch.h"
#include "PickingRenderer.h"
#include "DepthPrepass.h"

#include "Renderer/Swapchain.h"
#include "Renderer/CommandBuffer.h"
#include "Renderer/GraphicsDevice.h"

#include "World/Systems/PickingSystem.h"
#include "World/Systems/RenderSceneProxy.h"

using namespace Gleam;

void PickingRenderer::OnCreate(const RenderContext& context)
{
	mSwapchain = static_cast<Swapchain*>(context.surface);
	
	ComputePipelineStateDescriptor pipelineDesc;
	pipelineDesc.entryPoint = "visibilityPickingShader";
	mPipeline = context.device->CreateComputePipeline(pipelineDesc);
}

void PickingRenderer::OnDestroy(const RenderContext& context)
{

}

void PickingRenderer::AddRenderPasses(RenderGraph& graph, RenderGraphBlackboard& blackboard)
{
	auto frameIdx = mSwapchain->GetFrameIndex();
	auto pending = mSystem->AcquirePendingPick(frameIdx);
	if (pending == nullptr)
	{
		return;
	}

	const auto& sceneData = blackboard.Get<SceneRenderingData>();
	const auto& depthPrepassData = blackboard.Get<DepthPrepassData>();
	const auto& targetSize = graph.GetDescriptor(sceneData.sceneTarget).size;

	const auto targetWidth = static_cast<uint32_t>(targetSize.width);
	const auto targetHeight = static_cast<uint32_t>(targetSize.height);

	// The viewport may have resized between the request and this frame
	PickingRequest rect = pending->request;
	rect.width = rect.x < targetWidth ? Math::Min(rect.width, targetWidth - rect.x) : 0;
	rect.height = rect.y < targetHeight ? Math::Min(rect.height, targetHeight - rect.y) : 0;

	const auto& globalMeshes = sceneData.sceneProxy->GetGlobalMeshes();
	if (rect.width == 0 || rect.height == 0 || globalMeshes.empty())
	{
		mSystem->CompleteWithoutReadback(*pending);
		return;
	}

	// Instance indices are rebuilt every RenderSceneProxy tick, so resolve them against this frame
	pending->instanceToEntity.clear();
	pending->instanceToEntity.reserve(globalMeshes.size());
	for (const auto& meshInstance : globalMeshes)
	{
		pending->instanceToEntity.push_back(meshInstance.entity);
	}
	pending->encoded = true;

	struct PickingPassData
	{
		TextureHandle visibilityBuffer;
		BufferHandle maskBuffer;
	};

	graph.AddComputePass<PickingPassData>("Picking",
	[&](RenderGraphBuilder& builder, PickingPassData& passData)
	{
		BufferDescriptor maskDesc;
		maskDesc.name = "Picking Instance Mask";
		maskDesc.size = PickingSystem::MaskBytes;
		passData.maskBuffer = builder.WriteBuffer(builder.CreateBuffer(maskDesc));

		passData.visibilityBuffer = builder.ReadTexture(depthPrepassData.visibilityBuffer);
	},
	[this, rect, frameIdx](const CommandBuffer* cmd, const PickingPassData& passData)
	{
		Buffer maskBuffer = passData.maskBuffer;
		cmd->ClearBuffer(maskBuffer);

		BarrierGroup clearBarrier;
		clearBarrier.bufferBarriers.push_back({
			.resource  = maskBuffer.GetHandle(),
			.srcStage  = BarrierStage::ClearUnorderedAccess,
			.dstStage  = BarrierStage::ComputeShading,
			.srcAccess = BarrierAccess::UnorderedAccess,
			.dstAccess = BarrierAccess::UnorderedAccess,
		});
		cmd->Barrier(clearBarrier);

		PickingConstants constants = {};
		constants.visibilityBuffer = passData.visibilityBuffer;
		constants.maskBuffer = passData.maskBuffer;
		constants.rectOffsetX = rect.x;
		constants.rectOffsetY = rect.y;
		constants.rectWidth = rect.width;
		constants.rectHeight = rect.height;

		cmd->BindComputePipeline(mPipeline);
		cmd->SetPushConstant(constants);
		cmd->Dispatch(Math::DivideRoundingUp(rect.width, PICKING_GROUP_SIZE_X), Math::DivideRoundingUp(rect.height, PICKING_GROUP_SIZE_Y), 1);

		BarrierGroup copyBarrier;
		copyBarrier.bufferBarriers.push_back({
			.resource  = maskBuffer.GetHandle(),
			.srcStage  = BarrierStage::ComputeShading,
			.dstStage  = BarrierStage::Copy,
			.srcAccess = BarrierAccess::UnorderedAccess,
			.dstAccess = BarrierAccess::CopySource,
		});
		cmd->Barrier(copyBarrier);

		cmd->CopyBuffer(maskBuffer, mSystem->GetReadbackBuffer(), PickingSystem::MaskBytes, 0, frameIdx * PickingSystem::MaskBytes);

		// The graph tracks this buffer as UnorderedAccess and hands that stage to the allocator on
		// free, so restore it or a resource aliased onto this memory can start before the copy ends
		BarrierGroup restoreBarrier;
		restoreBarrier.bufferBarriers.push_back({
			.resource  = maskBuffer.GetHandle(),
			.srcStage  = BarrierStage::Copy,
			.dstStage  = BarrierStage::ComputeShading,
			.srcAccess = BarrierAccess::CopySource,
			.dstAccess = BarrierAccess::UnorderedAccess,
		});
		cmd->Barrier(restoreBarrier);
	});
}
