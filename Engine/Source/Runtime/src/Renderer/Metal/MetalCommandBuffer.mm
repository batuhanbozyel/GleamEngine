#include "gpch.h"

#ifdef USE_METAL_RENDERER
#include "Renderer/CommandBuffer.h"
#include "MetalPipelineStateManager.h"
#include "Renderer/Renderer.h"
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
    id<MTLRenderCommandEncoder> renderCommandEncoder = nil;
    id<MTLBlitCommandEncoder> blitCommandEncoder = nil;
    id<CAMetalDrawable> drawable = nil;
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
    mHandle->renderCommandEncoder = [mHandle->commandBuffer renderCommandEncoderWithDescriptor:renderPass];
}

void CommandBuffer::EndRenderPass() const
{
    [mHandle->renderCommandEncoder endEncoding];
    mHandle->renderCommandEncoder = nil;
}

void CommandBuffer::BindPipeline(const PipelineStateDescriptor& pipelineDesc, const GraphicsShader& program) const
{
    mHandle->pipelineState = MetalPipelineStateManager::GetGraphicsPipelineState(pipelineDesc, program);
    [mHandle->renderCommandEncoder setRenderPipelineState:mHandle->pipelineState.pipeline];
}

void CommandBuffer::SetViewport(uint32_t width, uint32_t height) const
{
    MTLViewport viewport;
    viewport.width = width;
    viewport.height = height;
    [mHandle->renderCommandEncoder setViewport:viewport];
}

void CommandBuffer::SetVertexBuffer(const Buffer& buffer, uint32_t index, uint32_t offset) const
{
    [mHandle->renderCommandEncoder setVertexBuffer:buffer.GetHandle() offset:offset atIndex:index];
}

void CommandBuffer::SetFragmentBuffer(const Buffer& buffer, uint32_t index, uint32_t offset) const
{
    [mHandle->renderCommandEncoder setFragmentBuffer:buffer.GetHandle() offset:offset atIndex:index];
}

void CommandBuffer::SetPushConstant(const void* data, uint32_t size, ShaderStage stage, uint32_t index) const
{
    switch (stage)
    {
        case ShaderStage::Vertex:
        {
            [mHandle->renderCommandEncoder setVertexBytes:data length:size atIndex:index];
            break;
        }
        case ShaderStage::Fragment:
        {
            [mHandle->renderCommandEncoder setFragmentBytes:data length:size atIndex:index];
            break;
        }
        case ShaderStage::Compute:
        {
            [mHandle->renderCommandEncoder setTileBytes:data length:size atIndex:index];
            break;
        }
    }
}

void CommandBuffer::Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t baseVertex, uint32_t baseInstance) const
{
    [mHandle->renderCommandEncoder drawPrimitives:PrimitiveToplogyToMTLPrimitiveType(mHandle->pipelineState.descriptor.topology) vertexStart:baseVertex vertexCount:vertexCount instanceCount:instanceCount baseInstance:baseInstance];
}

void CommandBuffer::DrawIndexed(const IndexBuffer& indexBuffer, uint32_t instanceCount, uint32_t firstIndex, uint32_t baseVertex, uint32_t baseInstance) const
{
    MTLIndexType indexType = static_cast<MTLIndexType>(indexBuffer.GetIndexType());
    [mHandle->renderCommandEncoder drawIndexedPrimitives:PrimitiveToplogyToMTLPrimitiveType(mHandle->pipelineState.descriptor.topology) indexCount:indexBuffer.GetCount() indexType:indexType indexBuffer:indexBuffer.GetHandle() indexBufferOffset:firstIndex instanceCount:instanceCount baseVertex:baseVertex baseInstance:baseInstance];
}

void CommandBuffer::CopyBuffer(const Buffer& src, const Buffer& dst, uint32_t size, uint32_t srcOffset, uint32_t dstOffset) const
{
    GLEAM_ASSERT(size <= src.GetSize(), "Metal: Copy size can not be larger than source buffer size!");
    GLEAM_ASSERT(size <= dst.GetSize(), "Metal: Copy size can not be larger than destination buffer size!");
    [mHandle->blitCommandEncoder copyFromBuffer:src.GetHandle() sourceOffset:srcOffset toBuffer:dst.GetHandle() destinationOffset:dstOffset size:size];
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

void CommandBuffer::WaitUntilCompleted() const
{
    [mHandle->commandBuffer waitUntilCompleted];
}

#endif

