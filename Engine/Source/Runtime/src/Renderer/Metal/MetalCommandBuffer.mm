#include "gpch.h"

#ifdef USE_METAL_RENDERER
#include "Renderer/CommandBuffer.h"
#include "MetalPipelineStateManager.h"

#include "Core/Application.h"
#include "Renderer/RendererBindingTable.h"
#include "Renderer/RenderTarget.h"

using namespace Gleam;

struct CommandBuffer::Impl
{
    id<MTLCommandBuffer> commandBuffer = nil;
    id<MTLRenderCommandEncoder> renderCommandEncoder = nil;
    RenderPassDescriptor renderPassDescriptor;
    MetalPipelineState pipelineState;
    bool swapchainTarget = false;
};

CommandBuffer::CommandBuffer()
    : mHandle(CreateScope<Impl>())
{
    
}

CommandBuffer::~CommandBuffer()
{
    [mHandle->commandBuffer waitUntilCompleted];
    mHandle->commandBuffer = nil;
}

void CommandBuffer::BeginRenderPass(const RenderPassDescriptor& renderPassDesc) const
{
    mHandle->renderPassDescriptor = renderPassDesc;
    mHandle->swapchainTarget = (mActiveRenderTarget == nullptr);
    MTLRenderPassDescriptor* renderPass = [MTLRenderPassDescriptor renderPassDescriptor];
    if (mHandle->swapchainTarget)
    {
        auto clearColor = ApplicationInstance.backgroundColor;
		if (renderPassDesc.attachments.size() > 0)
			clearColor = renderPassDesc.attachments[0].clearColor;

        id<CAMetalDrawable> drawable = MetalDevice::GetSwapchain().AcquireNextDrawable();
        MTLRenderPassColorAttachmentDescriptor* colorAttachmentDesc = renderPass.colorAttachments[0];
        colorAttachmentDesc.clearColor = { clearColor.r, clearColor.g, clearColor.b, clearColor.a };
        colorAttachmentDesc.loadAction = MTLLoadActionClear;
        colorAttachmentDesc.storeAction = MTLStoreActionStore;
        colorAttachmentDesc.texture = drawable.texture;
    }
    else
    {
        if (mActiveRenderTarget->HasDepthBuffer())
        {
            const auto& depthBuffer = mActiveRenderTarget->GetDepthBuffer();
            const auto& depthAttachment = renderPassDesc.attachments[renderPassDesc.depthAttachmentIndex];

            MTLRenderPassDepthAttachmentDescriptor* depthAttachmentDesc = renderPass.depthAttachment;
            depthAttachmentDesc.clearDepth = depthAttachment.clearDepth;
            depthAttachmentDesc.loadAction = AttachmentLoadActionToMTLLoadAction(depthAttachment.loadAction);
            depthAttachmentDesc.storeAction = AttachmentStoreActionToMTLStoreAction(depthAttachment.storeAction);
            depthAttachmentDesc.texture = depthBuffer->GetImageView();
            depthAttachmentDesc.resolveTexture = depthmap->GetHandle();
        }
        
        for (uint32_t i = 0; i <  mActiveRenderTarget->GetColorBuffers().size(); i++)
        {
            uint32_t attachmentIndexOffset = static_cast<int>(mActiveRenderTarget->HasDepthBuffer() && (renderPassDesc.depthAttachmentIndex <= i));
            uint32_t colorAttachmentIndex = i + attachmentIndexOffset;
            
            const auto& colorBuffer = mActiveRenderTarget->GetColorBuffers()[i];
            const auto& colorAttachment = renderPassDesc.attachments[colorAttachmentIndex];
            
            MTLRenderPassColorAttachmentDescriptor* colorAttachmentDesc = renderPass.colorAttachments[i];
            colorAttachmentDesc.clearColor =
            {
                colorAttachment.clearColor.r,
                colorAttachment.clearColor.g,
                colorAttachment.clearColor.b,
                colorAttachment.clearColor.a
            };
            colorAttachmentDesc.loadAction = AttachmentLoadActionToMTLLoadAction(colorAttachment.loadAction);
            colorAttachmentDesc.storeAction = AttachmentStoreActionToMTLStoreAction(colorAttachment.storeAction);
            colorAttachmentDesc.texture = colorBuffer->GetImageView();
            colorAttachmentDesc.resolveTexture = colorBuffer->GetHandle();
        }
    }
    mHandle->renderCommandEncoder = [mHandle->commandBuffer renderCommandEncoderWithDescriptor:renderPass];
}

void CommandBuffer::EndRenderPass() const
{
    [mHandle->renderCommandEncoder endEncoding];
    mHandle->renderCommandEncoder = nil;
}

void CommandBuffer::BindPipeline(const PipelineStateDescriptor& pipelineDesc, const GraphicsShader& program) const
{
    mHandle->pipelineState = MetalPipelineStateManager::GetGraphicsPipelineState(pipelineDesc, mHandle->renderPassDescriptor, program);
    [mHandle->renderCommandEncoder setRenderPipelineState:mHandle->pipelineState.pipeline];
    [mHandle->renderCommandEncoder setDepthStencilState:mHandle->pipelineState.depthStencil];
}

void CommandBuffer::SetViewport(const Size& size) const
{
    MTLViewport viewport{};
    viewport.width = size.width;
    viewport.height = size.height;
    viewport.zfar = 1.0f;
    [mHandle->renderCommandEncoder setViewport:viewport];
}

void CommandBuffer::SetVertexBuffer(const NativeGraphicsHandle buffer, BufferUsage usage, size_t size, uint32_t index, uint32_t offset) const
{
    [mHandle->renderCommandEncoder setVertexBuffer:buffer offset:offset atIndex:index];
}

void CommandBuffer::SetFragmentBuffer(const NativeGraphicsHandle buffer, BufferUsage usage, size_t size, uint32_t index, uint32_t offset) const
{
    [mHandle->renderCommandEncoder setFragmentBuffer:buffer offset:offset atIndex:index];
}

void CommandBuffer::SetPushConstant(const void* data, uint32_t size, ShaderStageFlagBits stage) const
{
    if (stage & ShaderStage_Vertex)
        [mHandle->renderCommandEncoder setVertexBytes:data length:size atIndex:RendererBindingTable::PushConstantBlock];
    
    if (stage & ShaderStage_Fragment)
        [mHandle->renderCommandEncoder setFragmentBytes:data length:size atIndex:RendererBindingTable::PushConstantBlock];
    
    if (stage & ShaderStage_Compute)
        [mHandle->renderCommandEncoder setTileBytes:data length:size atIndex:RendererBindingTable::PushConstantBlock];
}

void CommandBuffer::Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t baseVertex, uint32_t baseInstance) const
{
    [mHandle->renderCommandEncoder drawPrimitives:PrimitiveToplogyToMTLPrimitiveType(mHandle->pipelineState.descriptor.topology) vertexStart:baseVertex vertexCount:vertexCount instanceCount:instanceCount baseInstance:baseInstance];
}

void CommandBuffer::DrawIndexed(const NativeGraphicsHandle indexBuffer, IndexType type, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, uint32_t baseVertex, uint32_t baseInstance) const
{
    MTLIndexType indexType = static_cast<MTLIndexType>(type);
    [mHandle->renderCommandEncoder drawIndexedPrimitives:PrimitiveToplogyToMTLPrimitiveType(mHandle->pipelineState.descriptor.topology) indexCount:indexCount indexType:indexType indexBuffer:indexBuffer indexBufferOffset:firstIndex * SizeOfIndexType(type) instanceCount:instanceCount baseVertex:baseVertex baseInstance:baseInstance];
}

void CommandBuffer::CopyBuffer(const IBuffer& src, const IBuffer& dst, size_t size, uint32_t srcOffset, uint32_t dstOffset) const
{
    id<MTLBlitCommandEncoder> blitCommandEncoder = [mHandle->commandBuffer blitCommandEncoder];
    [blitCommandEncoder copyFromBuffer:src.GetHandle() sourceOffset:srcOffset toBuffer:dst.GetHandle() destinationOffset:dstOffset size:size];
    [blitCommandEncoder endEncoding];
}

void CommandBuffer::Blit(const RenderTexture& texture, const Optional<RenderTexture>& target) const
{
    mHandle->swapchainTarget = !target.has_value();
    id<MTLBlitCommandEncoder> blitCommandEncoder = [mHandle->commandBuffer blitCommandEncoder];
    id targetTexture = mHandle->swapchainTarget ? MetalDevice::GetSwapchain().AcquireNextDrawable().texture : target.value().GetHandle();
    [blitCommandEncoder copyFromTexture:texture.GetHandle() toTexture:targetTexture];
    [blitCommandEncoder endEncoding];
}

void CommandBuffer::Begin() const
{
    mHandle->commandBuffer = [MetalDevice::GetCommandPool() commandBuffer];
}

void CommandBuffer::End() const
{
    [mHandle->commandBuffer enqueue];
}

void CommandBuffer::Commit() const
{
    [mHandle->commandBuffer commit];
    mHandle->swapchainTarget = false;
}

void CommandBuffer::Present() const
{
    MetalDevice::GetSwapchain().Present(mHandle->commandBuffer);
    [mHandle->commandBuffer commit];
}

void CommandBuffer::WaitUntilCompleted() const
{
    [mHandle->commandBuffer waitUntilCompleted];
}

#endif
