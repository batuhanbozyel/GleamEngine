#include "gpch.h"

#ifdef USE_DIRECTX_RENDERER
#include "Renderer/CommandBuffer.h"

#include "DirectXPipelineStateManager.h"
#include "DirectXTransitionManager.h"
#include "DirectXShaderReflect.h"
#include "DirectXDevice.h"
#include "DirectXUtils.h"

using namespace Gleam;

struct CommandBuffer::Impl
{
	DirectXDevice* device = nullptr;

	const DirectXPipeline* pipeline = nullptr;
	DirectXCommandList commandList = {};
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
}

void CommandBuffer::BeginRenderPass(const RenderPassDescriptor& renderPassDesc, const TStringView debugName) const
{
    mHandle->sampleCount = renderPassDesc.samples;
	mHandle->hasDepthAttachment = renderPassDesc.depthAttachment.texture.IsValid();

	D3D12_RENDER_PASS_DEPTH_STENCIL_DESC depthAttachment{};
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

		if (colorAttachmentDesc.texture.IsValid())
		{
			auto resource = static_cast<ID3D12Resource*>(colorAttachmentDesc.texture.GetHandle());
			colorAttachments[i].cpuDescriptor; // TODO:
			DirectXTransitionManager::TransitionLayout(mHandle->commandList.handle, resource, D3D12_RESOURCE_STATE_RENDER_TARGET);
		}
		else
		{
			const auto& drawable = mHandle->device->AcquireNextDrawable();
			colorAttachments[i].cpuDescriptor = drawable.descriptor;

			D3D12_RESOURCE_BARRIER barrier{};
			barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
			barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
			barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
			barrier.Transition.pResource = drawable.renderTarget;
			mHandle->commandList.handle->ResourceBarrier(1, &barrier);
		}

		// TODO: implement MSAA resolve -> olorAttachments[i].EndingAccess.Resolve
	}

	if (mHandle->hasDepthAttachment)
	{
		auto format = renderPassDesc.depthAttachment.texture.GetDescriptor().format;
		auto resource = static_cast<ID3D12Resource*>(renderPassDesc.depthAttachment.texture.GetHandle());
		DirectXTransitionManager::TransitionLayout(mHandle->commandList.handle, resource, D3D12_RESOURCE_STATE_DEPTH_WRITE);

		depthAttachment.cpuDescriptor; // TODO:
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
	}
	mHandle->commandList.handle->BeginRenderPass(colorAttachments.size(), colorAttachments.data(), &depthAttachment, D3D12_RENDER_PASS_FLAG_NONE);
}

void CommandBuffer::EndRenderPass() const
{
	mHandle->commandList.handle->EndRenderPass();
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
	mHandle->commandList.handle->SetPipelineState(mHandle->pipeline->handle);
	mHandle->commandList.handle->IASetPrimitiveTopology(PrimitiveToplogyToD3D_PRIMITIVE_TOPOLOGY(pipelineDesc.topology));
	mHandle->commandList.handle->SetGraphicsRootSignature(DirectXPipelineStateManager::GetGlobalRootSignature());
	mHandle->commandList.handle->OMSetStencilRef(pipelineDesc.stencilState.reference);

	const auto& cbvSrvUavHeap = mHandle->device->GetCbvSrvUavHeap();
	mHandle->commandList.handle->SetDescriptorHeaps(1, &cbvSrvUavHeap.handle);
	mHandle->commandList.handle->SetGraphicsRootDescriptorTable(D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE, cbvSrvUavHeap.gpuHandle);
}

void CommandBuffer::SetViewport(const Size& size) const
{
	D3D12_VIEWPORT viewport{};
	viewport.MaxDepth = 1.0f;
	viewport.Width = size.width;
	viewport.Height = size.height;
	mHandle->commandList.handle->RSSetViewports(1, &viewport);

	D3D12_RECT scissor{};
	scissor.right = static_cast<uint32_t>(size.width);
	scissor.bottom = static_cast<uint32_t>(size.height);
	mHandle->commandList.handle->RSSetScissorRects(1, &scissor);
}

void CommandBuffer::BindBuffer(const Buffer& buffer,
	size_t offset,
	uint32_t index,
	ShaderStageFlagBits stage,
	ResourceAccess access) const
{
	const auto resource = [&, this]()
	{
		const Shader::Reflection* reflection = nullptr;
		if (stage & ShaderStage_Vertex)
		{
			auto pipeline = static_cast<const DirectXGraphicsPipeline*>(mHandle->pipeline);
			reflection = pipeline->vertexShader.GetReflection();
		}
		else if (stage & ShaderStage_Fragment)
		{
			auto pipeline = static_cast<const DirectXGraphicsPipeline*>(mHandle->pipeline);
			reflection = pipeline->fragmentShader.GetReflection();
		}
		else
		{
			GLEAM_ASSERT(false, "DirectX: Shader stage not implemented yet.")
		}

		switch (buffer.GetDescriptor().usage)
		{
			case BufferUsage::UniformBuffer: return Shader::Reflection::GetResourceFromTypeArray(reflection->CBVs, index);
			case BufferUsage::VertexBuffer:
			case BufferUsage::StorageBuffer:
			{
                switch(access)
                {
                    case ResourceAccess::Read: return Shader::Reflection::GetResourceFromTypeArray(reflection->SRVs, index);
                    case ResourceAccess::Write: return Shader::Reflection::GetResourceFromTypeArray(reflection->UAVs, index);
                    default: GLEAM_ASSERT(false, "DirectX: Trying to bind buffer with invalid access.")
					{
						return Shader::Reflection::invalidResource;
					}
                }
			}
			default: GLEAM_ASSERT(false, "DirectX: Trying to bind buffer with invalid usage.")
			{
				return Shader::Reflection::invalidResource;
			}
		}
	}();
	if (resource.Type == Shader::Reflection::invalidResource.Type) return;

	// TODO: update descriptor heap
}

void CommandBuffer::BindTexture(const Texture& texture,
	uint32_t index,
	ShaderStageFlagBits stage,
	ResourceAccess access) const
{
	const auto resource = [&, this]()
	{
		const Shader::Reflection* reflection = nullptr;
		if (stage & ShaderStage_Vertex)
		{
			auto pipeline = static_cast<const DirectXGraphicsPipeline*>(mHandle->pipeline);
			reflection = pipeline->vertexShader.GetReflection();
		}
		else if (stage & ShaderStage_Fragment)
		{
			auto pipeline = static_cast<const DirectXGraphicsPipeline*>(mHandle->pipeline);
			reflection = pipeline->fragmentShader.GetReflection();
		}
		else
		{
			GLEAM_ASSERT(false, "DirectX: Shader stage not implemented yet.")
		}
        switch(access)
        {
            case ResourceAccess::Read: return Shader::Reflection::GetResourceFromTypeArray(reflection->SRVs, index);
            case ResourceAccess::Write: return Shader::Reflection::GetResourceFromTypeArray(reflection->UAVs, index);
			default: GLEAM_ASSERT(false, "DirectX: Trying to bind texture with invalid access.")
			{
				return Shader::Reflection::invalidResource;
			}
        }
	}();
	if (resource.Type == Shader::Reflection::invalidResource.Type) return;

	auto imageLayout = [access]()
	{
		switch (access)
		{
			case ResourceAccess::Read: return D3D12_RESOURCE_STATE_GENERIC_READ;
			case ResourceAccess::Write: return D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
			default: return D3D12_RESOURCE_STATE_COMMON;
		}
	}();
	DirectXTransitionManager::TransitionLayout(mHandle->commandList.handle, static_cast<ID3D12Resource*>(texture.GetHandle()), imageLayout);

	// TODO: update descriptor heap
}

void CommandBuffer::SetPushConstant(const void* data, uint32_t size, ShaderStageFlagBits stage) const
{
	mHandle->commandList.handle->SetGraphicsRoot32BitConstants(D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS, size / sizeof(uint32_t), data, 0);
}

void CommandBuffer::Draw(uint32_t vertexCount,
	uint32_t instanceCount,
	uint32_t baseVertex,
	uint32_t baseInstance) const
{
	mHandle->commandList.handle->DrawInstanced(vertexCount, instanceCount, baseVertex, baseInstance);
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
	indexBufferView.SizeInBytes = indexBuffer.GetDescriptor().size;

	mHandle->commandList.handle->IASetIndexBuffer(&indexBufferView);
	mHandle->commandList.handle->DrawIndexedInstanced(indexCount, instanceCount, firstIndex, baseVertex, baseInstance);
}

void CommandBuffer::CopyBuffer(const NativeGraphicsHandle src, const NativeGraphicsHandle dst,
	size_t size,
	size_t srcOffset,
	size_t dstOffset) const
{
	auto srcBuffer = static_cast<ID3D12Resource*>(src);
	auto dstBuffer = static_cast<ID3D12Resource*>(dst);
	mHandle->commandList.handle->CopyBufferRegion(srcBuffer, dstOffset, dstBuffer, srcOffset, size);
}

void CommandBuffer::Blit(const Texture& texture, const Texture& target) const
{
    auto swapchainTarget = !target.IsValid();
	auto targetTexture = swapchainTarget ? mHandle->device->AcquireNextDrawable().renderTarget : static_cast<ID3D12Resource*>(target.GetHandle());

	if (swapchainTarget)
	{
		DirectXTransitionManager::TransitionLayout(mHandle->commandList.handle, targetTexture, D3D12_RESOURCE_STATE_COPY_DEST);
	}

	D3D12_TEXTURE_COPY_LOCATION dst{};
	dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
	dst.pResource = targetTexture;
	dst.SubresourceIndex = 0;

	D3D12_TEXTURE_COPY_LOCATION src{};
	dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
	src.pResource = static_cast<ID3D12Resource*>(texture.GetHandle());
	src.SubresourceIndex = 0;
	mHandle->commandList.handle->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);

	if (swapchainTarget)
	{
		DirectXTransitionManager::TransitionLayout(mHandle->commandList.handle, targetTexture, D3D12_RESOURCE_STATE_PRESENT);
	}
}

void CommandBuffer::Begin() const
{
	mHandle->commandList = mHandle->device->AllocateCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT);
	mCommitted = false;
}

void CommandBuffer::End() const
{
	mHandle->commandList.handle->Close();
}

void CommandBuffer::Commit() const
{
	ID3D12CommandList* commandList = mHandle->commandList.handle;
	mHandle->device->GetDirectQueue()->ExecuteCommandLists(1, &commandList);
	mHandle->device->GetDirectQueue()->Signal(mHandle->fence, mHandle->fenceValue);
	mStagingHeap.Reset();
	mCommitted = true;
}

void CommandBuffer::WaitUntilCompleted() const
{
	if (mCommitted)
	{
		WaitForID3D12Fence(mHandle->fence, mHandle->fenceValue++);
	}
}

NativeGraphicsHandle CommandBuffer::GetHandle() const
{
	return mHandle->commandList.handle;
}

NativeGraphicsHandle CommandBuffer::GetActiveRenderPass() const
{
	return nullptr;
}

#endif
