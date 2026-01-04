#include "gpch.h"

#ifdef USE_DIRECTX_RENDERER
#include "Renderer/CommandBuffer.h"

#include "DirectXDevice.h"
#include "DirectXUtils.h"

using namespace Gleam;

struct CommandBuffer::Impl
{
	DirectXDevice* device = nullptr;

	ID3D12GraphicsCommandList7* commandList = nullptr;
	ID3D12Fence* fence = nullptr;
	uint64_t fenceValue = 1;
	uint64_t waitFenceValue = 0;

	PipelineHandle pipeline;
};

CommandBuffer::CommandBuffer(GraphicsDevice* device)
	: mHandle(CreateScope<Impl>())
	, mDevice(device)
	, mConstantBuffer(device, 4194304) // 4 MB
{
	mHandle->device = static_cast<DirectXDevice*>(device);
	DX_CHECK(static_cast<ID3D12Device10*>(mHandle->device->GetHandle())->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mHandle->fence)));
}

CommandBuffer::~CommandBuffer()
{
	WaitUntilCompleted();
	mHandle->fence->Release();
}

void CommandBuffer::BeginRenderPass(const RenderPassDescriptor& renderPassDesc, const TStringView debugName) const
{
	PIXBeginEvent(mHandle->commandList, PIX_COLOR(128, 255, 128), debugName.data());
	TArray<D3D12_RENDER_PASS_RENDER_TARGET_DESC> colorAttachments(renderPassDesc.colorAttachments.size());
	for (uint32_t i = 0; i < colorAttachments.size(); ++i)
	{
		const auto& colorAttachmentDesc = renderPassDesc.colorAttachments[i];
		auto format = colorAttachmentDesc.texture.GetDescriptor().format;

		colorAttachments[i].BeginningAccess.Type = AttachmentLoadActionToDX_TYPE(colorAttachmentDesc.loadAction);
		colorAttachments[i].BeginningAccess.Clear.ClearValue.Format = TextureFormatToDXGI_FORMAT(format);
		colorAttachments[i].BeginningAccess.Clear.ClearValue.Color[0] = colorAttachmentDesc.clearColor.r;
		colorAttachments[i].BeginningAccess.Clear.ClearValue.Color[1] = colorAttachmentDesc.clearColor.g;
		colorAttachments[i].BeginningAccess.Clear.ClearValue.Color[2] = colorAttachmentDesc.clearColor.b;
		colorAttachments[i].BeginningAccess.Clear.ClearValue.Color[3] = colorAttachmentDesc.clearColor.a;
		colorAttachments[i].EndingAccess.Type = AttachmentStoreActionToDX_TYPE(colorAttachmentDesc.storeAction);

		auto resource = static_cast<ID3D12Resource*>(colorAttachmentDesc.texture.GetHandle());
		colorAttachments[i].cpuDescriptor = colorAttachmentDesc.texture.GetRenderTargetView();
	}

	if (renderPassDesc.depthAttachment.texture.IsValid())
	{
		auto format = renderPassDesc.depthAttachment.texture.GetDescriptor().format;
		auto resource = static_cast<ID3D12Resource*>(renderPassDesc.depthAttachment.texture.GetHandle());

		D3D12_RENDER_PASS_DEPTH_STENCIL_DESC depthAttachment{};
		depthAttachment.cpuDescriptor = renderPassDesc.depthAttachment.texture.GetRenderTargetView();
		depthAttachment.DepthBeginningAccess.Type = AttachmentLoadActionToDX_TYPE(renderPassDesc.depthAttachment.loadAction);
		depthAttachment.DepthBeginningAccess.Clear.ClearValue.Format = TextureFormatToDXGI_FORMAT(format);
		depthAttachment.DepthBeginningAccess.Clear.ClearValue.DepthStencil.Depth = renderPassDesc.depthAttachment.clearDepth;
		depthAttachment.DepthBeginningAccess.Clear.ClearValue.DepthStencil.Stencil = renderPassDesc.depthAttachment.clearStencil;
		depthAttachment.DepthEndingAccess.Type = AttachmentStoreActionToDX_TYPE(renderPassDesc.depthAttachment.storeAction);

		if (Utils::IsDepthStencilFormat(format))
		{
			depthAttachment.StencilBeginningAccess.Type = depthAttachment.DepthBeginningAccess.Type;
			depthAttachment.StencilBeginningAccess.Clear.ClearValue.Format = depthAttachment.DepthBeginningAccess.Clear.ClearValue.Format;
			depthAttachment.StencilBeginningAccess.Clear.ClearValue.DepthStencil.Depth = renderPassDesc.depthAttachment.clearDepth;
			depthAttachment.StencilBeginningAccess.Clear.ClearValue.DepthStencil.Stencil = renderPassDesc.depthAttachment.clearStencil;
			depthAttachment.StencilEndingAccess.Type = depthAttachment.DepthEndingAccess.Type;
		}
		else
		{
			depthAttachment.StencilBeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_NO_ACCESS;
			depthAttachment.StencilEndingAccess.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_NO_ACCESS;
		}
		mHandle->commandList->BeginRenderPass((UINT)colorAttachments.size(), colorAttachments.data(), &depthAttachment, D3D12_RENDER_PASS_FLAG_NONE);
	}
	else
	{
		mHandle->commandList->BeginRenderPass((UINT)colorAttachments.size(), colorAttachments.data(), nullptr, D3D12_RENDER_PASS_FLAG_NONE);
	}
}

void CommandBuffer::EndRenderPass() const
{
	mHandle->commandList->EndRenderPass();
	PIXEndEvent(mHandle->commandList);
}

void CommandBuffer::BeginComputePass(const TStringView debugName) const
{
	PIXBeginEvent(mHandle->commandList, PIX_COLOR(255, 165, 0), debugName.data());
}

void CommandBuffer::EndComputePass() const
{
	PIXEndEvent(mHandle->commandList);
}

void CommandBuffer::BindComputePipeline(const ComputePipeline& pipeline) const
{
	const auto& cbvSrvUavHeap = mHandle->device->GetCbvSrvUavHeap();
	mHandle->commandList->SetDescriptorHeaps(1, &cbvSrvUavHeap.handle);
	mHandle->commandList->SetComputeRootSignature(mHandle->device->GetGlobalRootSignature());
	mHandle->commandList->SetPipelineState(static_cast<ID3D12PipelineState*>(pipeline.GetHandle()));
	mHandle->pipeline = pipeline.GetHash();
}

void CommandBuffer::BindGraphicsPipeline(const GraphicsPipeline& pipeline) const
{
	const auto& cbvSrvUavHeap = mHandle->device->GetCbvSrvUavHeap();
	mHandle->commandList->SetDescriptorHeaps(1, &cbvSrvUavHeap.handle);
	mHandle->commandList->SetGraphicsRootSignature(mHandle->device->GetGlobalRootSignature());

	mHandle->commandList->SetPipelineState(static_cast<ID3D12PipelineState*>(pipeline.GetHandle()));
	mHandle->commandList->OMSetStencilRef(pipeline.GetDescriptor().stencilState.reference);
	mHandle->commandList->IASetPrimitiveTopology(PrimitiveToplogyToD3D_PRIMITIVE_TOPOLOGY(pipeline.GetDescriptor().topology));
	mHandle->pipeline = pipeline.GetHash();
}

void CommandBuffer::SetViewport(const Size& size) const
{
	D3D12_VIEWPORT viewport{};
	viewport.MaxDepth = 1.0f;
	viewport.Width = size.width;
	viewport.Height = size.height;
	mHandle->commandList->RSSetViewports(1, &viewport);
}

void CommandBuffer::SetScissorRect(const Rect& rect) const
{
	D3D12_RECT scissor{};
	scissor.left = static_cast<uint32_t>(rect.offset.x);
	scissor.top = static_cast<uint32_t>(rect.offset.y);
	scissor.right = static_cast<uint32_t>(rect.size.width + rect.offset.x);
	scissor.bottom = static_cast<uint32_t>(rect.size.height + rect.offset.y);
	mHandle->commandList->RSSetScissorRects(1, &scissor);
}

void CommandBuffer::SetConstantBuffer(const void* data, uint32_t size, uint32_t slot) const
{
	auto gpuAddress = static_cast<ID3D12Resource*>(mConstantBuffer.GetHandle())->GetGPUVirtualAddress(); 
	gpuAddress += mConstantBuffer.Write(data, size);

	if (mHandle->pipeline.type == PipelineType::Compute)
	{
		mHandle->commandList->SetComputeRootConstantBufferView(slot, gpuAddress);
	}
	else // if (mHandle->pipeline.type == PipelineType::Graphics)
	{
		mHandle->commandList->SetGraphicsRootConstantBufferView(slot, gpuAddress);
	}
}

void CommandBuffer::SetPushConstant(const void* data, uint32_t size) const
{
	if (mHandle->pipeline.type == PipelineType::Compute)
	{
		mHandle->commandList->SetComputeRoot32BitConstants(PUSH_CONSTANT_SLOT, size / sizeof(uint32_t), data, 0);
	}
	else // if (mHandle->pipeline.type == PipelineType::Graphics)
	{
		mHandle->commandList->SetGraphicsRoot32BitConstants(PUSH_CONSTANT_SLOT, size / sizeof(uint32_t), data, 0);
	}
}

void CommandBuffer::Dispatch(uint32_t x, uint32_t y, uint32_t z) const
{
	mHandle->commandList->Dispatch(x, y, z);
}

void CommandBuffer::Draw(uint32_t vertexCount, uint32_t instanceCount) const
{
	mHandle->commandList->DrawInstanced(vertexCount, instanceCount, 0, 0);
}

void CommandBuffer::DrawIndexed(const Buffer& indexBuffer, IndexType type,
	uint32_t indexCount,
	uint32_t instanceCount,
	uint32_t firstIndex,
	uint32_t baseVertex) const
{
	D3D12_INDEX_BUFFER_VIEW indexBufferView = {};
	indexBufferView.BufferLocation = static_cast<ID3D12Resource*>(indexBuffer.GetHandle())->GetGPUVirtualAddress();
	indexBufferView.Format = type == IndexType::UINT16 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
	indexBufferView.SizeInBytes = (UINT)indexBuffer.GetSize();

	mHandle->commandList->IASetIndexBuffer(&indexBufferView);
	mHandle->commandList->DrawIndexedInstanced(indexCount, instanceCount, firstIndex, baseVertex, 0);
}

void CommandBuffer::CopyBuffer(const NativeGraphicsHandle src, const NativeGraphicsHandle dst,
	size_t size,
	size_t srcOffset,
	size_t dstOffset) const
{
	auto srcBuffer = static_cast<ID3D12Resource*>(src);
	auto dstBuffer = static_cast<ID3D12Resource*>(dst);
	mHandle->commandList->CopyBufferRegion(dstBuffer, dstOffset, srcBuffer, srcOffset, size);
}

void CommandBuffer::Blit(const Texture& source, const Texture& destination) const
{
    auto swapchainTarget = destination.IsValid() == false;
	auto srcTexture = static_cast<ID3D12Resource*>(source.GetHandle());
	auto dstTexture = static_cast<ID3D12Resource*>(destination.GetHandle());

	D3D12_TEXTURE_COPY_LOCATION dst{};
	dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
	dst.pResource = dstTexture;
	dst.SubresourceIndex = 0;

	D3D12_TEXTURE_COPY_LOCATION src{};
	dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
	src.pResource = srcTexture;
	src.SubresourceIndex = 0;
	mHandle->commandList->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);
}

void CommandBuffer::Barrier(const BarrierGroup& barrier) const
{
	TArray<D3D12_BUFFER_BARRIER> d3d12BufferBarriers;
	TArray<D3D12_TEXTURE_BARRIER> d3d12TextureBarriers;

	d3d12BufferBarriers.reserve(barrier.bufferBarriers.size());
	d3d12TextureBarriers.reserve(barrier.textureBarriers.size());

	for (const BufferBarrier& bufferBarrier : barrier.bufferBarriers)
	{
		D3D12_BUFFER_BARRIER d3d12Barrier = {};
		d3d12Barrier.SyncBefore = BarrierStageToD3D12_BARRIER_SYNC(bufferBarrier.srcStage);
		d3d12Barrier.SyncAfter = BarrierStageToD3D12_BARRIER_SYNC(bufferBarrier.dstStage);
		d3d12Barrier.AccessBefore = BarrierAccessToD3D12_BARRIER_ACCESS(bufferBarrier.srcAccess);
		d3d12Barrier.AccessAfter = BarrierAccessToD3D12_BARRIER_ACCESS(bufferBarrier.dstAccess);
		d3d12Barrier.pResource = static_cast<ID3D12Resource*>(bufferBarrier.resource);
		d3d12Barrier.Offset = 0;
		d3d12Barrier.Size = UINT64_MAX;
		d3d12BufferBarriers.emplace_back(d3d12Barrier);
	}

	for (const TextureBarrier& textureBarrier : barrier.textureBarriers)
	{
		D3D12_TEXTURE_BARRIER d3d12Barrier = {};
		d3d12Barrier.SyncBefore = BarrierStageToD3D12_BARRIER_SYNC(textureBarrier.srcStage);
		d3d12Barrier.SyncAfter = BarrierStageToD3D12_BARRIER_SYNC(textureBarrier.dstStage);
		d3d12Barrier.AccessBefore = BarrierAccessToD3D12_BARRIER_ACCESS(textureBarrier.srcAccess);
		d3d12Barrier.AccessAfter = BarrierAccessToD3D12_BARRIER_ACCESS(textureBarrier.dstAccess);
		d3d12Barrier.LayoutBefore = BarrierLayoutToD3D12_BARRIER_LAYOUT(textureBarrier.oldLayout);
		d3d12Barrier.LayoutAfter = BarrierLayoutToD3D12_BARRIER_LAYOUT(textureBarrier.newLayout);
		d3d12Barrier.pResource = static_cast<ID3D12Resource*>(textureBarrier.resource);
		d3d12Barrier.Subresources.IndexOrFirstMipLevel = 0xffffffff;
		d3d12Barrier.Subresources.NumMipLevels = 0;
		d3d12Barrier.Subresources.FirstArraySlice = 0;
		d3d12Barrier.Subresources.NumArraySlices = 0;
		d3d12Barrier.Subresources.FirstPlane = 0;
		d3d12Barrier.Subresources.NumPlanes = 0;
		d3d12Barrier.Flags = D3D12_TEXTURE_BARRIER_FLAG_NONE;
		d3d12TextureBarriers.emplace_back(d3d12Barrier);
	}

	TArray<D3D12_BARRIER_GROUP> barrierGroups;
	if (not d3d12BufferBarriers.empty())
	{
		D3D12_BARRIER_GROUP bufferGroup = {};
		bufferGroup.Type = D3D12_BARRIER_TYPE_BUFFER;
		bufferGroup.NumBarriers = static_cast<UINT32>(d3d12BufferBarriers.size());
		bufferGroup.pBufferBarriers = d3d12BufferBarriers.data();
		barrierGroups.emplace_back(bufferGroup);
	}

	if (not d3d12TextureBarriers.empty())
	{
		D3D12_BARRIER_GROUP textureGroup = {};
		textureGroup.Type = D3D12_BARRIER_TYPE_TEXTURE;
		textureGroup.NumBarriers = static_cast<UINT32>(d3d12TextureBarriers.size());
		textureGroup.pTextureBarriers = d3d12TextureBarriers.data();
		barrierGroups.emplace_back(textureGroup);
	}
	mHandle->commandList->Barrier(static_cast<UINT32>(barrierGroups.size()), barrierGroups.data());
}

void CommandBuffer::Begin(const TStringView debugName) const
{
	TWString debugNameW = StringUtils::Convert(debugName);
	mHandle->commandList = mHandle->device->AllocateCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT, debugNameW);
	mCommitted = false;
}

void CommandBuffer::End() const
{
	mHandle->commandList->Close();
}

void CommandBuffer::Commit() const
{
	mHandle->waitFenceValue = mHandle->fenceValue;

	ID3D12CommandList* commandList = mHandle->commandList;
	mHandle->device->GetDirectQueue()->ExecuteCommandLists(1, &commandList);
	mHandle->device->GetDirectQueue()->Signal(mHandle->fence, mHandle->fenceValue++);
	mConstantBuffer.Reset();
	mCommitted = true;
}

void CommandBuffer::WaitUntilCompleted() const
{
	if (mCommitted)
	{
		WaitForID3D12Fence(mHandle->fence, mHandle->waitFenceValue);
	}
	mCommitted = false;
}

NativeGraphicsHandle CommandBuffer::GetHandle() const
{
	return mHandle->commandList;
}

#endif
