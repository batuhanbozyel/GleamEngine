#include "gpch.h"

#ifdef USE_DIRECTX_RENDERER
#include "Renderer/CommandBuffer.h"

#include "DirectXPipelineStateManager.h"
#include "DirectXTransitionManager.h"
#include "DirectXShaderReflect.h"
#include "DirectXDevice.h"
#include "DirectXUtils.h"

#include "Core/Application.h"

using namespace Gleam;

struct CommandBuffer::Impl
{
	DirectXDevice* device = nullptr;

	const DirectXPipeline* pipeline = nullptr;
	ID3D12GraphicsCommandList7* commandList = nullptr;
	ID3D12Fence* fence = nullptr;
	uint32_t frameIdx = 0;

	TArray<TextureDescriptor> colorAttachments;
	TextureDescriptor depthAttachment;
	bool hasDepthAttachment = false;
	uint32_t sampleCount = 1;
};

CommandBuffer::CommandBuffer(GraphicsDevice* device)
	: mHandle(CreateScope<Impl>()), mDevice(device)
{
	mHandle->device = static_cast<DirectXDevice*>(device);
	mHandle->frameIdx = device->GetFrameIndex();
    
    HeapDescriptor descriptor;
    descriptor.size = 4194304; // 4 MB;
    descriptor.memoryType = MemoryType::CPU;
    mStagingHeap = mDevice->CreateHeap(descriptor);

	DX_CHECK(static_cast<ID3D12Device10*>(mHandle->device->GetHandle())->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mHandle->fence)));
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

		colorAttachments[i].cpuDescriptor = rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
		colorAttachments[i].BeginningAccess.Type = AttachmentLoadActionToDX_TYPE(colorAttachmentDesc.loadAction);
		colorAttachments[i].BeginningAccess.Clear.ClearValue.Format = TextureFormatToDXGI_FORMAT(format);
		colorAttachments[i].BeginningAccess.Clear.ClearValue.Color[0] = colorAttachmentDesc.clearColor.r;
		colorAttachments[i].BeginningAccess.Clear.ClearValue.Color[1] = colorAttachmentDesc.clearColor.g;
		colorAttachments[i].BeginningAccess.Clear.ClearValue.Color[2] = colorAttachmentDesc.clearColor.b;
		colorAttachments[i].BeginningAccess.Clear.ClearValue.Color[3] = colorAttachmentDesc.clearColor.a;
		colorAttachments[i].EndingAccess.Type = AttachmentStoreActionToDX_TYPE(colorAttachmentDesc.storeAction);
		// TODO: implement MSAA resolve -> olorAttachments[i].EndingAccess.Resolve
	}

	if (mHandle->hasDepthAttachment)
	{
		auto format = renderPassDesc.depthAttachment.texture.GetDescriptor().format;

		depthAttachment.cpuDescriptor = dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
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
	mHandle->commandList->BeginRenderPass(colorAttachments.size(), colorAttachments.data(), &depthAttachment, D3D12_RENDER_PASS_FLAG_NONE);
}

void CommandBuffer::EndRenderPass() const
{
	mHandle->commandList->EndRenderPass();
}

void CommandBuffer::BindGraphicsPipeline(const PipelineStateDescriptor& pipelineDesc, const Shader& vertexShader, const Shader& fragmentShader) const
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

	// Set vertex samplers
	size_t samplerCount = vertexShader.GetReflection()->samplers.size() + fragmentShader.GetReflection()->samplers.size();
	if (samplerCount > 0)
	{
		TArray<VkDescriptorImageInfo> samplerInfos;
		samplerInfos.reserve(samplerCount);

		TArray<VkWriteDescriptorSet> descriptorSets;
		descriptorSets.reserve(samplerCount);
		for (const auto& resource : vertexShader.GetReflection()->samplers)
		{
			VkDescriptorImageInfo samplerInfo{};
			samplerInfo.sampler = VulkanPipelineStateManager::GetSampler(resource->binding);

			VkWriteDescriptorSet descriptorSet{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
			descriptorSet.dstBinding = resource->binding;
			descriptorSet.descriptorCount = 1;
			descriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
			descriptorSet.pImageInfo = &samplerInfos.emplace_back(samplerInfo);
			descriptorSets.emplace_back(descriptorSet);
		}

		// Set fragment samplers
		for (const auto& resource : fragmentShader.GetReflection()->samplers)
		{
			VkDescriptorImageInfo samplerInfo{};
			samplerInfo.sampler = VulkanPipelineStateManager::GetSampler(resource->binding);

			VkWriteDescriptorSet descriptorSet{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
			descriptorSet.dstBinding = resource->binding;
			descriptorSet.descriptorCount = 1;
			descriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
			descriptorSet.pImageInfo = &samplerInfos.emplace_back(samplerInfo);
			descriptorSets.emplace_back(descriptorSet);
		}
		vkCmdPushDescriptorSetKHR(mHandle->commandBuffer, mHandle->pipeline->bindPoint, mHandle->pipeline->layout, 0, static_cast<uint32_t>(descriptorSets.size()), descriptorSets.data());
	}
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

void CommandBuffer::BindBuffer(const NativeGraphicsHandle buffer, BufferUsage usage, size_t offset, uint32_t index, ShaderStageFlagBits stage, ResourceAccess access) const
{
	const auto resource = [=, this]()
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
			GLEAM_ASSERT(false, "Metal: Shader stage not implemented yet.")
		}

		switch (usage)
		{
			case BufferUsage::UniformBuffer: return Shader::Reflection::GetResourceFromTypeArray(reflection->CBVs, index);
			case BufferUsage::VertexBuffer:
			case BufferUsage::StorageBuffer:
			{
                switch(access)
                {
                    case ResourceAccess::Read: return Shader::Reflection::GetResourceFromTypeArray(reflection->SRVs, index + Shader::Reflection::SRV_BINDING_OFFSET);
                    case ResourceAccess::Write: return Shader::Reflection::GetResourceFromTypeArray(reflection->UAVs, index + Shader::Reflection::UAV_BINDING_OFFSET);
                    default: GLEAM_ASSERT(false, "DirectX: Trying to bind buffer with invalid access.")
					{
						return (const SpvReflectDescriptorBinding*)nullptr;
					}
                }
			}
			default: GLEAM_ASSERT(false, "DirectX: Trying to bind buffer with invalid usage.")
			{
				return (const SpvReflectDescriptorBinding*)nullptr;
			}
		}
	}();
	if (resource == nullptr) return;

	VkDescriptorBufferInfo bufferInfo{};
	bufferInfo.buffer = static_cast<VkBuffer>(buffer);
	bufferInfo.offset = offset;
	bufferInfo.range = VK_WHOLE_SIZE;

	VkWriteDescriptorSet descriptorSet{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
	descriptorSet.dstBinding = resource->binding;
	descriptorSet.descriptorCount = 1;
	descriptorSet.descriptorType = SpvReflectDescriptorTypeToVkDescriptorType(resource->descriptor_type);
	descriptorSet.pBufferInfo = &bufferInfo;
	vkCmdPushDescriptorSetKHR(mHandle->commandBuffer, mHandle->pipeline->bindPoint, mHandle->pipeline->layout, resource->set, 1, &descriptorSet);
}

void CommandBuffer::BindTexture(const NativeGraphicsHandle texture, uint32_t index, ShaderStageFlagBits stage, ResourceAccess access) const
{
	const auto resource = [=, this]()
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
            case ResourceAccess::Read: return Shader::Reflection::GetResourceFromTypeArray(reflection->SRVs, index + Shader::Reflection::SRV_BINDING_OFFSET);
            case ResourceAccess::Write: return Shader::Reflection::GetResourceFromTypeArray(reflection->UAVs, index + Shader::Reflection::UAV_BINDING_OFFSET);
			default: GLEAM_ASSERT(false, "DirectX: Trying to bind texture with invalid access.")
			{
				return (const SpvReflectDescriptorBinding*)nullptr;
			}
        }
	}();
	if (resource == nullptr) return;

	auto imageLayout = [=]()
	{
		switch (access)
		{
			case ResourceAccess::Read: return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			case ResourceAccess::Write: return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			default: return VK_IMAGE_LAYOUT_GENERAL;
		}
	}();

	VkDescriptorImageInfo imageInfo{};
	imageInfo.imageView = static_cast<VkImageView>(texture);
	imageInfo.imageLayout = imageLayout;

	VkWriteDescriptorSet descriptorSet{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
	descriptorSet.dstBinding = resource->binding;
	descriptorSet.descriptorCount = 1;
	descriptorSet.descriptorType = SpvReflectDescriptorTypeToVkDescriptorType(resource->descriptor_type);
	descriptorSet.pImageInfo = &imageInfo;
	vkCmdPushDescriptorSetKHR(mHandle->commandBuffer, mHandle->pipeline->bindPoint, mHandle->pipeline->layout, resource->set, 1, &descriptorSet);
}

void CommandBuffer::SetPushConstant(const void* data, uint32_t size, ShaderStageFlagBits stage) const
{
    VkShaderStageFlags flagBits{};
    
    if (stage & ShaderStage_Vertex)
        flagBits |= VK_SHADER_STAGE_VERTEX_BIT;
    
    if (stage & ShaderStage_Fragment)
        flagBits |= VK_SHADER_STAGE_FRAGMENT_BIT;
    
    if (stage & ShaderStage_Compute)
        flagBits |= VK_SHADER_STAGE_COMPUTE_BIT;
    
    vkCmdPushConstants(mHandle->commandBuffer, mHandle->pipeline->layout, flagBits, 0, size, data);
}

void CommandBuffer::Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t baseVertex, uint32_t baseInstance) const
{
	mHandle->commandList->DrawInstanced(vertexCount, instanceCount, baseVertex, baseInstance);
}

void CommandBuffer::DrawIndexed(const Buffer& indexBuffer, IndexType type, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, uint32_t baseVertex, uint32_t baseInstance) const
{
	D3D12_INDEX_BUFFER_VIEW indexBufferView = {};
	indexBufferView.BufferLocation = static_cast<ID3D12Resource*>(indexBuffer.GetHandle())->GetGPUVirtualAddress();
	indexBufferView.Format = type == IndexType::UINT16 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
	indexBufferView.SizeInBytes = indexBuffer.GetDescriptor().size;

	mHandle->commandList->IASetIndexBuffer(&indexBufferView);
	mHandle->commandList->DrawIndexedInstanced(indexCount, instanceCount, firstIndex, baseVertex, baseInstance);
}

void CommandBuffer::CopyBuffer(const NativeGraphicsHandle src, const NativeGraphicsHandle dst, size_t size, size_t srcOffset, size_t dstOffset) const
{
	VkBufferCopy bufferCopy{};
	bufferCopy.srcOffset = srcOffset;
	bufferCopy.dstOffset = dstOffset;
	bufferCopy.size = size;
	vkCmdCopyBuffer(mHandle->commandBuffer, static_cast<VkBuffer>(src), static_cast<VkBuffer>(dst), 1, &bufferCopy);
}

void CommandBuffer::Blit(const Texture& texture, const Texture& target) const
{
    mHandle->swapchainTarget = !target.IsValid();
	VkImage targetTexture = mHandle->swapchainTarget ? mHandle->swapchain->AcquireNextDrawable().image : static_cast<VkImage>(target.GetHandle());

	if (mHandle->swapchainTarget)
	{
		DirectXTransitionManager::TransitionLayout(mHandle->commandList, targetTexture, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	}

	VkImageCopy region{};
	region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.srcSubresource.layerCount = 1;
	region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.dstSubresource.layerCount = 1;
	region.extent.width = static_cast<uint32_t>(texture.GetDescriptor().size.width);
	region.extent.height = static_cast<uint32_t>(texture.GetDescriptor().size.height);
	region.extent.depth = 1;
	vkCmdCopyImage(mHandle->commandBuffer, static_cast<VkImage>(texture.GetHandle()), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, targetTexture, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

	if (mHandle->swapchainTarget)
	{
		VulkanTransitionManager::TransitionLayout(mHandle->commandBuffer, targetTexture, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
	}
}

void CommandBuffer::TransitionLayout(const Texture& texture, ResourceAccess access) const
{
	auto imageLayout = [=]()
	{
		switch (access)
		{
			case ResourceAccess::Read: return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			case ResourceAccess::Write: return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			default: return VK_IMAGE_LAYOUT_GENERAL;
		}
	}();
	DirectXTransitionManager::TransitionLayout(mHandle->commandList, texture.GetHandle(), imageLayout);
}

void CommandBuffer::Begin() const
{
	mHandle->commandBuffer = mHandle->swapchain->AllocateCommandBuffer();
	VkCommandBufferBeginInfo beginInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	VK_CHECK(vkBeginCommandBuffer(mHandle->commandBuffer, &beginInfo));
	VK_CHECK(vkResetFences(static_cast<VkDevice>(mHandle->device->GetHandle()), 1, &mHandle->fence));
	mCommitted = false;
}

void CommandBuffer::End() const
{
	VK_CHECK(vkEndCommandBuffer(mHandle->commandBuffer));
}

void CommandBuffer::Commit() const
{
	ID3D12CommandList* commandList = mHandle->commandList;
	mHandle->device->GetDirectQueue()->ExecuteCommandLists(1, &commandList);
	mStagingHeap.Reset();
	mCommitted = true;
}

void CommandBuffer::Present() const
{
	VkPipelineStageFlags waitDstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	VkSemaphore waitSemaphore = mHandle->swapchain->GetImageAcquireSemaphore();
	VkSemaphore signalSemaphore = mHandle->swapchain->GetImageReleaseSemaphore();

	VkSubmitInfo submitInfo{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &waitSemaphore;
	submitInfo.pWaitDstStageMask = &waitDstStageMask;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &mHandle->commandBuffer;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &signalSemaphore;
	VK_CHECK(vkQueueSubmit(mHandle->device->GetGraphicsQueue().handle, 1, &submitInfo, mHandle->fence));
	mStagingHeap.Reset();
	mCommitted = true;

	mHandle->swapchain->Present();
}

void CommandBuffer::WaitUntilCompleted() const
{
	if (mCommitted)
	{
		VK_CHECK(vkWaitForFences(static_cast<VkDevice>(mHandle->device->GetHandle()), 1, &mHandle->fence, VK_TRUE, UINT64_MAX));
	}
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
