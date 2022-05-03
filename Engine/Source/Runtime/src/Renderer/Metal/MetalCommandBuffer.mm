#include "gpch.h"

#ifdef USE_METAL_RENDERER
#include "Renderer/CommandBuffer.h"
#include "MetalUtils.h"

#include "Renderer/Buffer.h"
#include "Renderer/Shader.h"
#include "Renderer/RenderPassDescriptor.h"
#include "Renderer/PipelineStateDescriptor.h"

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
    TArray<GraphicsPipelineCacheElement> graphicsPipelineCache;
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
    MTLRenderPassDescriptor* renderPass = [MTLRenderPassDescriptor renderPassDescriptor];
    for (uint32_t i = 0; i < renderPassDesc.attachments.size(); i++)
    {
        const auto& attachmentDesc = renderPassDesc.attachments[i];
        MTLRenderPassColorAttachmentDescriptor* colorAttachmentDesc = renderPass.colorAttachments[i];
        colorAttachmentDesc.clearColor = MTLClearColorMake(attachmentDesc.clearColor.r, attachmentDesc.clearColor.g, attachmentDesc.clearColor.b, attachmentDesc.clearColor.a);
        if (attachmentDesc.swapchainTarget)
        {
            const auto& frame = RendererContext::GetSwapchain()->GetCurrentFrame();
            colorAttachmentDesc.loadAction = MTLLoadActionClear;
            colorAttachmentDesc.storeAction = MTLStoreActionStore;
            colorAttachmentDesc.texture = frame.drawable.texture;
        }
    }
    mHandle->commandEncoder = [mHandle->commandBuffer renderCommandEncoderWithDescriptor:renderPass];
    
    ApplyRenderPipeline(renderPassDesc, pipelineDesc, program);
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
    mHandle->commandBuffer = [id<MTLCommandQueue>(RendererContext::GetSwapchain()->GetGraphicsCommandPool(0)) commandBuffer];
}

void CommandBuffer::Commit() const
{
    [mHandle->commandBuffer commit];
}

void CommandBuffer::Present() const
{
    const auto& frame = RendererContext::GetSwapchain()->GetCurrentFrame();
    [mHandle->commandBuffer addCompletedHandler:^(id<MTLCommandBuffer> commandBuffer)
    {
        dispatch_semaphore_signal(frame.imageAcquireSemaphore);
    }];
    
    [mHandle->commandBuffer presentDrawable:frame.drawable];
}

void CommandBuffer::ApplyRenderPipeline(const RenderPassDescriptor& renderPassDesc, const PipelineStateDescriptor& pipelineDesc, const GraphicsShader& program) const
{
    for (const auto& element : mHandle->graphicsPipelineCache)
    {
        if (element.renderPassDescriptor == renderPassDesc
            && element.pipelineStateDescriptor == pipelineDesc
            && element.program == program)
        {
            [mHandle->commandEncoder setRenderPipelineState:element.pipeline];
            return;
        }
    }
    
    MTLRenderPipelineDescriptor* pipelineDescriptor = [MTLRenderPipelineDescriptor new];
    pipelineDescriptor.vertexFunction = program.vertexShader->GetHandle();
    pipelineDescriptor.fragmentFunction = program.fragmentShader->GetHandle();
    pipelineDescriptor.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;
    
    /**
            TODO:
     */
    
    NSError* error;
    id<MTLRenderPipelineState> pipeline = [MetalDevice newRenderPipelineStateWithDescriptor:pipelineDescriptor error:&error];
    
    GraphicsPipelineCacheElement element;
    element.pipelineStateDescriptor = pipelineDesc;
    element.renderPassDescriptor = renderPassDesc;
    element.program = program;
    element.pipeline = pipeline;
    mHandle->graphicsPipelineCache.push_back(element);
    [mHandle->commandEncoder setRenderPipelineState:element.pipeline];
}

#endif

