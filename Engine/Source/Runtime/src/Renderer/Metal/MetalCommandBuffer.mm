#include "gpch.h"

#ifdef USE_METAL_RENDERER
#include "Renderer/CommandBuffer.h"
#include "MetalPipelineStateManager.h"

#include "Renderer/Buffer.h"

using namespace Gleam;

struct GraphicsPipelineCacheElement
{
    RenderPassDescriptor renderPassDescriptor;
    PipelineStateDescriptor pipelineStateDescriptor;
    GraphicsShader program;
    id<MTLRenderPipelineState> pipeline = nil;
};

struct CommandBuffer::Impl
{
    id<MTLCommandBuffer> commandBuffer = nil;
    id<MTLRenderCommandEncoder> commandEncoder = nil;
    MetalPipelineState pipelineState;
    bool swapchainTarget = false;
};

CommandBuffer::CommandBuffer()
    : mHandle(CreateScope<Impl>())
{
    
}

CommandBuffer::~CommandBuffer()
{
    mHandle->commandBuffer = nil;
}

void CommandBuffer::BeginRenderPass(const RenderPassDescriptor& renderPassDesc, const PipelineStateDescriptor& pipelineDesc, const GraphicsShader& program) const
{
    mHandle->pipelineState = MetalPipelineStateManager::GetGraphicsPipelineState(pipelineDesc, program);
    mHandle->swapchainTarget = renderPassDesc.swapchainTarget;
    
    MTLRenderPassDescriptor* renderPass = [MTLRenderPassDescriptor renderPassDescriptor];
    if (renderPassDesc.swapchainTarget)
    {
        MTLClearColor clearValue;
        if (renderPassDesc.attachments.size() > 0)
        {
            const auto& clearColor = renderPassDesc.attachments[0].clearColor;
            clearValue = { clearColor.r, clearColor.g, clearColor.b, clearColor.a };
        }
        else
        {
            const auto& clearColor = Renderer::clearColor;
            clearValue = { clearColor.r, clearColor.g, clearColor.b, clearColor.a };
        }
        
        id<CAMetalDrawable> drawable = RendererContext::GetSwapchain()->AcquireNextDrawable();
        MTLRenderPassColorAttachmentDescriptor* colorAttachmentDesc = renderPass.colorAttachments[0];
        colorAttachmentDesc.clearColor = clearValue;
        colorAttachmentDesc.loadAction = MTLLoadActionClear;
        colorAttachmentDesc.storeAction = MTLStoreActionStore;
        colorAttachmentDesc.texture = drawable.texture;
    }
    else
    {
        // TODO:
    }
    mHandle->commandEncoder = [mHandle->commandBuffer renderCommandEncoderWithDescriptor:renderPass];
    
    [mHandle->commandEncoder setRenderPipelineState:mHandle->pipelineState.pipeline];
}

void CommandBuffer::EndRenderPass() const
{
    [mHandle->commandEncoder endEncoding];
    mHandle->commandEncoder = nil;
}

void CommandBuffer::SetViewport(uint32_t width, uint32_t height) const
{
    MTLViewport viewport;
    viewport.width = width;
    viewport.height = height;
    [mHandle->commandEncoder setViewport:viewport];
}

void CommandBuffer::SetVertexBuffer(const Buffer& buffer, uint32_t index, uint32_t offset) const
{
    [mHandle->commandEncoder setVertexBuffer:buffer.GetHandle() offset:offset atIndex:index];
}

void CommandBuffer::SetFragmentBuffer(const Buffer& buffer, uint32_t index, uint32_t offset) const
{
    [mHandle->commandEncoder setFragmentBuffer:buffer.GetHandle() offset:offset atIndex:index];
}

void CommandBuffer::SetPushConstant(const void* data, uint32_t size, ShaderStage stage, uint32_t index) const
{
    switch (stage)
    {
        case ShaderStage::Vertex:
        {
            [mHandle->commandEncoder setVertexBytes:data length:size atIndex:index];
            break;
        }
        case ShaderStage::Fragment:
        {
            [mHandle->commandEncoder setFragmentBytes:data length:size atIndex:index];
            break;
        }
        case ShaderStage::Compute:
        {
            [mHandle->commandEncoder setTileBytes:data length:size atIndex:index];
            break;
        }
    }
    
}

void CommandBuffer::Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t baseVertex, uint32_t baseInstance) const
{
    [mHandle->commandEncoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:baseVertex vertexCount:vertexCount instanceCount:instanceCount baseInstance:baseInstance];
}

void CommandBuffer::DrawIndexed(const IndexBuffer& indexBuffer, uint32_t instanceCount, uint32_t firstIndex, uint32_t baseVertex, uint32_t baseInstance) const
{
    MTLIndexType indexType = static_cast<MTLIndexType>(indexBuffer.GetIndexType());
    [mHandle->commandEncoder drawIndexedPrimitives:MTLPrimitiveTypeTriangle indexCount:indexBuffer.GetCount() indexType:indexType indexBuffer:indexBuffer.GetHandle() indexBufferOffset:firstIndex instanceCount:instanceCount baseVertex:baseVertex baseInstance:baseInstance];
}

void CommandBuffer::Begin() const
{
    mHandle->commandBuffer = [id<MTLCommandQueue>(RendererContext::GetGraphicsCommandPool(0)) commandBuffer];
}

void CommandBuffer::End() const
{
    [mHandle->commandBuffer enqueue];
}

void CommandBuffer::Commit() const
{
    if (mHandle->swapchainTarget)
	{
		RendererContext::GetSwapchain()->Present(mHandle->commandBuffer);
	}
    else
	{
		// TODO:
	}
}

#endif

