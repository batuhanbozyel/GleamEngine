#include "gpch.h"

#ifdef USE_DIRECTX_RENDERER
#include "Renderer/CommandBuffer.h"

#include "DirectXPipelineStateManager.h"
#include "DirectXTransitionManager.h"
#include "DirectXDevice.h"
#include "DirectXUtils.h"

using namespace Gleam;

struct CommandBuffer::Impl
{
	DirectXDevice* device = nullptr;

	const DirectXPipeline* pipeline = nullptr;
	ID3D12GraphicsCommandList7* commandList = nullptr;
	ID3D12Fence* fence = nullptr;
	uint32_t fenceValue = 0;

	TArray<TextureDescriptor> colorAttachments;
	TextureDescriptor depthAttachment;
	bool hasDepthAttachment = false;
	uint32_t sampleCount = 1;
};

CommandBuffer::CommandBuffer(GraphicsDevice* device)
	: mHandle(CreateScope<Impl>()), mDevice(device)
{
	mHandle->device = static_cast<DirectXDevice*>(device);
    
    HeapDescriptor descriptor;
    descriptor.size = 4194304; // 4 MB;
    descriptor.memoryType = MemoryType::CPU;
    mStagingHeap = mDevice->CreateHeap(descriptor);

	DX_CHECK(static_cast<ID3D12Device10*>(mHandle->device->GetHandle())->CreateFence(
		mHandle->fenceValue,
		D3D12_FENCE_FLAG_NONE,
		IID_PPV_ARGS(&mHandle->fence)
	));
}

CommandBuffer::~CommandBuffer()
{
	mDevice->Dispose(mStagingHeap);
	mHandle->fence->Release();

	mHandle->pipeline = nullptr;
	mHandle->commandList = nullptr;
	mHandle->fence = nullptr;
}

void CommandBuffer::BeginRenderPass(const RenderPassDescriptor& renderPassDesc, const TStringView debugName) const
{
    mHandle->sampleCount = renderPassDesc.samples;
	mHandle->hasDepthAttachment = renderPassDesc.depthAttachment.texture.IsValid();

	TArray<D3D12_RENDER_PASS_RENDER_TARGET_DESC> colorAttachments(renderPassDesc.colorAttachments.size());
	mHandle->colorAttachments.resize(renderPassDesc.colorAttachments.size());
	for (uint32_t i = 0; i < colorAttachments.size(); ++i)
	{
		const auto& colorAttachmentDesc = renderPassDesc.colorAttachments[i];
		auto format = colorAttachmentDesc.texture.GetDescriptor().format;
		mHandle->colorAttachments[i] = colorAttachmentDesc.texture.GetDescriptor();

		colorAttachments[i].BeginningAccess.Type = AttachmentLoadActionToDX_TYPE(colorAttachmentDesc.loadAction);
		colorAttachments[i].BeginningAccess.Clear.ClearValue.Format = TextureFormatToDXGI_FORMAT(format);
		colorAttachments[i].BeginningAccess.Clear.ClearValue.Color[0] = colorAttachmentDesc.clearColor.r;
		colorAttachments[i].BeginningAccess.Clear.ClearValue.Color[1] = colorAttachmentDesc.clearColor.g;
		colorAttachments[i].BeginningAccess.Clear.ClearValue.Color[2] = colorAttachmentDesc.clearColor.b;
		colorAttachments[i].BeginningAccess.Clear.ClearValue.Color[3] = colorAttachmentDesc.clearColor.a;
		colorAttachments[i].EndingAccess.Type = AttachmentStoreActionToDX_TYPE(colorAttachmentDesc.storeAction);

		if (colorAttachmentDesc.texture.IsValid())
		{
			auto resource = static_cast<ID3D12Resource*>(colorAttachmentDesc.texture.GetHandle());

		#ifdef GDEBUG
			TStringStream resourceName;
			resourceName << debugName << "::colorAttachment_" << i;
			resource->SetName(StringUtils::Convert(resourceName.str()).data());
		#endif

			colorAttachments[i].cpuDescriptor = colorAttachmentDesc.texture.GetView();
			DirectXTransitionManager::TransitionLayout(mHandle->commandList,
				resource, D3D12_RESOURCE_STATE_RENDER_TARGET);
		}
		else
		{
			const auto& drawable = mHandle->device->AcquireNextDrawable();
			colorAttachments[i].cpuDescriptor = drawable.view;
			DirectXTransitionManager::TransitionLayout(mHandle->commandList,
				drawable.renderTarget, D3D12_RESOURCE_STATE_RENDER_TARGET);
		}

		// TODO: implement MSAA resolve -> olorAttachments[i].EndingAccess.Resolve
	}

	if (mHandle->hasDepthAttachment)
	{
		mHandle->depthAttachment = renderPassDesc.depthAttachment.texture.GetDescriptor();
		auto format = renderPassDesc.depthAttachment.texture.GetDescriptor().format;
		auto resource = static_cast<ID3D12Resource*>(renderPassDesc.depthAttachment.texture.GetHandle());

	#ifdef GDEBUG
		TStringStream resourceName;
		resourceName << debugName << "::depthAttachment";
		resource->SetName(StringUtils::Convert(resourceName.str()).data());
	#endif

		DirectXTransitionManager::TransitionLayout(mHandle->commandList, resource, D3D12_RESOURCE_STATE_DEPTH_WRITE);

		D3D12_RENDER_PASS_DEPTH_STENCIL_DESC depthAttachment{};
		depthAttachment.cpuDescriptor = renderPassDesc.depthAttachment.texture.GetView();
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

void CommandBuffer::BindGraphicsPipeline(const PipelineStateDescriptor& pipelineDesc,
	const Shader& vertexShader,
	const Shader& fragmentShader) const
{
	if (mHandle->hasDepthAttachment)
	{
		mHandle->pipeline = DirectXPipelineStateManager::GetGraphicsPipeline(pipelineDesc, mHandle->colorAttachments, mHandle->depthAttachment, vertexShader, fragmentShader, mHandle->sampleCount);
	}
	else
	{
		mHandle->pipeline = DirectXPipelineStateManager::GetGraphicsPipeline(pipelineDesc, mHandle->colorAttachments, vertexShader, fragmentShader, mHandle->sampleCount);
	}
	mHandle->commandList->SetPipelineState(mHandle->pipeline->handle);
	mHandle->commandList->IASetPrimitiveTopology(PrimitiveToplogyToD3D_PRIMITIVE_TOPOLOGY(pipelineDesc.topology));
	mHandle->commandList->SetGraphicsRootSignature(DirectXPipelineStateManager::GetGlobalRootSignature());
	mHandle->commandList->OMSetStencilRef(pipelineDesc.stencilState.reference);

	const auto& cbvSrvUavHeap = mHandle->device->GetCbvSrvUavHeap();
	mHandle->commandList->SetDescriptorHeaps(1, &cbvSrvUavHeap.handle);
}

void CommandBuffer::SetViewport(const Size& size) const
{
	D3D12_VIEWPORT viewport{};
	viewport.MaxDepth = 1.0f;
	viewport.Width = size.width;
	viewport.Height = size.height;
	mHandle->commandList->RSSetViewports(1, &viewport);

	D3D12_RECT scissor{};
	scissor.right = static_cast<uint32_t>(size.width);
	scissor.bottom = static_cast<uint32_t>(size.height);
	mHandle->commandList->RSSetScissorRects(1, &scissor);
}

void CommandBuffer::SetConstantBuffer(const void* data, uint32_t size, uint32_t slot) const
{
	auto buffer = mStagingHeap.CreateBuffer(size);
	SetBufferData(buffer, data, size);

    mHandle->commandList->SetGraphicsRootConstantBufferView(slot, static_cast<ID3D12Resource*>(buffer.GetHandle())->GetGPUVirtualAddress());
}

void CommandBuffer::SetPushConstant(const void* data, uint32_t size) const
{
	mHandle->commandList->SetGraphicsRoot32BitConstants(PUSH_CONSTANT_SLOT, size / sizeof(uint32_t), data, 0);
}

void CommandBuffer::Draw(uint32_t vertexCount,
	uint32_t instanceCount,
	uint32_t baseVertex,
	uint32_t baseInstance) const
{
	mHandle->commandList->DrawInstanced(vertexCount, instanceCount, baseVertex, baseInstance);
}

void CommandBuffer::DrawIndexed(const Buffer& indexBuffer, IndexType type,
	uint32_t indexCount,
	uint32_t instanceCount,
	uint32_t firstIndex,
	uint32_t baseVertex,
	uint32_t baseInstance) const
{
	D3D12_INDEX_BUFFER_VIEW indexBufferView = {};
	indexBufferView.BufferLocation = static_cast<ID3D12Resource*>(indexBuffer.GetHandle())->GetGPUVirtualAddress();
	indexBufferView.Format = type == IndexType::UINT16 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
	indexBufferView.SizeInBytes = (UINT)indexBuffer.GetSize();

	mHandle->commandList->IASetIndexBuffer(&indexBufferView);
	mHandle->commandList->DrawIndexedInstanced(indexCount, instanceCount, firstIndex, baseVertex, baseInstance);
}

void CommandBuffer::CopyBuffer(const NativeGraphicsHandle src, const NativeGraphicsHandle dst,
	size_t size,
	size_t srcOffset,
	size_t dstOffset) const
{
	auto srcBuffer = static_cast<ID3D12Resource*>(src);
	auto dstBuffer = static_cast<ID3D12Resource*>(dst);

	DirectXTransitionManager::TransitionLayout(mHandle->commandList, dstBuffer, D3D12_RESOURCE_STATE_COPY_DEST);
	mHandle->commandList->CopyBufferRegion(dstBuffer, dstOffset, srcBuffer, srcOffset, size);
	DirectXTransitionManager::TransitionLayout(mHandle->commandList, dstBuffer, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
}

void CommandBuffer::Blit(const Texture& source, const Texture& destination) const
{
    auto swapchainTarget = destination.IsValid() == false;
	auto srcTexture = static_cast<ID3D12Resource*>(source.GetHandle());
	auto dstTexture = swapchainTarget ? mHandle->device->AcquireNextDrawable().renderTarget : static_cast<ID3D12Resource*>(destination.GetHandle());

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

	D3D12_RESOURCE_STATES finalLayout = D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE;
	if (swapchainTarget)
	{
		finalLayout = D3D12_RESOURCE_STATE_RENDER_TARGET;
	}

	DirectXTransitionManager::TransitionLayout(mHandle->commandList, srcTexture, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
	DirectXTransitionManager::TransitionLayout(mHandle->commandList, dstTexture, finalLayout);
}

void CommandBuffer::Begin() const
{
	mHandle->commandList = mHandle->device->AllocateCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT);
#ifdef GDEBUG
	TStringStream resourceName;
	resourceName << "CommandList::Direct_" << mHandle->device->GetFrameIndex();
	mHandle->commandList->SetName(StringUtils::Convert(resourceName.str()).data());
#endif
	mCommitted = false;
}

void CommandBuffer::End() const
{
	mHandle->commandList->Close();
}

void CommandBuffer::Commit() const
{
	ID3D12CommandList* commandList = mHandle->commandList;
	mHandle->device->GetDirectQueue()->ExecuteCommandLists(1, &commandList);
	mHandle->device->GetDirectQueue()->Signal(mHandle->fence, ++mHandle->fenceValue);
	mStagingHeap.Reset();
	mCommitted = true;
}

void CommandBuffer::WaitUntilCompleted() const
{
	if (mCommitted)
	{
		WaitForID3D12Fence(mHandle->fence, mHandle->fenceValue);
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
