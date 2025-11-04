#include "gpch.h"

#ifdef USE_DIRECTX_RENDERER
#include "Renderer/CommandBuffer.h"

#include "DirectXTransitionManager.h"
#include "DirectXDevice.h"
#include "DirectXUtils.h"

using namespace Gleam;

struct CommandBuffer::Impl
{
	DirectXDevice* device = nullptr;

	ID3D12GraphicsCommandList7* commandList = nullptr;
	ID3D12Fence* fence = nullptr;
	uint64_t fenceValue = 1;
	uint64_t waitFenceValue = 1;
};

CommandBuffer::CommandBuffer(GraphicsDevice* device)
	: mHandle(CreateScope<Impl>())
	, mDevice(device)
	, mConstantBuffer(device, 4194304) // 4 MB
{
	mHandle->device = static_cast<DirectXDevice*>(device);
	DX_CHECK(static_cast<ID3D12Device10*>(mHandle->device->GetHandle())->CreateFence(
		mHandle->fenceValue,
		D3D12_FENCE_FLAG_NONE,
		IID_PPV_ARGS(&mHandle->fence)
	));
}

CommandBuffer::~CommandBuffer()
{
	WaitUntilCompleted();
	mHandle->fence->Release();
}

void CommandBuffer::BeginRenderPass(const RenderPassDescriptor& renderPassDesc, const TStringView debugName) const
{
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
		DirectXTransitionManager::TransitionLayout(mHandle->commandList,
			resource, D3D12_RESOURCE_STATE_RENDER_TARGET);
	}

	if (renderPassDesc.depthAttachment.texture.IsValid())
	{
		auto format = renderPassDesc.depthAttachment.texture.GetDescriptor().format;
		auto resource = static_cast<ID3D12Resource*>(renderPassDesc.depthAttachment.texture.GetHandle());
		DirectXTransitionManager::TransitionLayout(mHandle->commandList, resource, D3D12_RESOURCE_STATE_DEPTH_WRITE);

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
}

void CommandBuffer::BindGraphicsPipeline(const GraphicsPipeline& pipeline) const
{
	const auto& cbvSrvUavHeap = mHandle->device->GetCbvSrvUavHeap();
	mHandle->commandList->SetDescriptorHeaps(1, &cbvSrvUavHeap.handle);
	mHandle->commandList->SetGraphicsRootSignature(mHandle->device->GetGlobalRootSignature());

	mHandle->commandList->SetPipelineState(static_cast<ID3D12PipelineState*>(pipeline.GetHandle()));
	mHandle->commandList->OMSetStencilRef(pipeline.GetDescriptor().stencilState.reference);
	mHandle->commandList->IASetPrimitiveTopology(PrimitiveToplogyToD3D_PRIMITIVE_TOPOLOGY(pipeline.GetDescriptor().topology));
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
    mHandle->commandList->SetGraphicsRootConstantBufferView(slot, gpuAddress);
}

void CommandBuffer::SetPushConstant(const void* data, uint32_t size) const
{
	mHandle->commandList->SetGraphicsRoot32BitConstants(PUSH_CONSTANT_SLOT, size / sizeof(uint32_t), data, 0);
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

	DirectXTransitionManager::TransitionLayout(mHandle->commandList, dstBuffer, D3D12_RESOURCE_STATE_COPY_DEST);
	DirectXTransitionManager::TransitionLayout(mHandle->commandList, srcBuffer, D3D12_RESOURCE_STATE_COPY_SOURCE);
	
	mHandle->commandList->CopyBufferRegion(dstBuffer, dstOffset, srcBuffer, srcOffset, size);
	
	DirectXTransitionManager::TransitionLayout(mHandle->commandList, srcBuffer, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
	DirectXTransitionManager::TransitionLayout(mHandle->commandList, dstBuffer, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
}

void CommandBuffer::Blit(const Texture& source, const Texture& destination) const
{
    auto swapchainTarget = destination.IsValid() == false;
	auto srcTexture = static_cast<ID3D12Resource*>(source.GetHandle());
	auto dstTexture = static_cast<ID3D12Resource*>(destination.GetHandle());

	DirectXTransitionManager::TransitionLayout(mHandle->commandList, dstTexture, D3D12_RESOURCE_STATE_COPY_DEST);
	DirectXTransitionManager::TransitionLayout(mHandle->commandList, srcTexture, D3D12_RESOURCE_STATE_COPY_SOURCE);

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
	mHandle->waitFenceValue = mHandle->fenceValue++;

	ID3D12CommandList* commandList = mHandle->commandList;
	mHandle->device->GetDirectQueue()->ExecuteCommandLists(1, &commandList);
	mHandle->device->GetDirectQueue()->Signal(mHandle->fence, mHandle->fenceValue);
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

NativeGraphicsHandle CommandBuffer::GetActiveRenderPass() const
{
	return nullptr;
}

#endif
