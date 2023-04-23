#include "gpch.h"

#ifdef USE_METAL_RENDERER
#include "Renderer/CommandBuffer.h"
#include "MetalPipelineStateManager.h"

#include "Core/Game.h"
#include "Renderer/RendererBindingTable.h"

using namespace Gleam;

struct CommandBuffer::Impl
{
    id<MTLCommandBuffer> commandBuffer = nil;
    id<MTLRenderCommandEncoder> renderCommandEncoder = nil;
    MetalPipelineState pipelineState;
    
    TArray<TextureDescriptor> colorAttachments;
    TextureDescriptor depthAttachment;
    bool hasDepthAttachment = false;
    uint32_t sampleCount = 1;
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
    MTLRenderPassDescriptor* renderPass = [MTLRenderPassDescriptor renderPassDescriptor];
    
    mHandle->sampleCount = renderPassDesc.samples;
    mHandle->hasDepthAttachment = renderPassDesc.depthAttachment.texture != nullptr;
    if (renderPassDesc.depthAttachment.texture)
    {
        mHandle->depthAttachment = renderPassDesc.depthAttachment.texture->GetDescriptor();
        
        if (Utils::IsDepthFormat(mHandle->depthAttachment.format))
        {
            MTLRenderPassDepthAttachmentDescriptor* depthAttachmentDesc = renderPass.depthAttachment;
            depthAttachmentDesc.clearDepth = renderPassDesc.depthAttachment.clearDepth;
            depthAttachmentDesc.loadAction = AttachmentLoadActionToMTLLoadAction(renderPassDesc.depthAttachment.loadAction);
            depthAttachmentDesc.storeAction = AttachmentStoreActionToMTLStoreAction(renderPassDesc.depthAttachment.storeAction);
            depthAttachmentDesc.texture = renderPassDesc.depthAttachment.texture->GetRenderSurface();
            
            if (renderPassDesc.samples > 1)
                depthAttachmentDesc.resolveTexture = renderPassDesc.depthAttachment.texture->GetHandle();
        }
        
        if (Utils::IsStencilFormat(mHandle->depthAttachment.format))
        {
            MTLRenderPassStencilAttachmentDescriptor* stencilAttachmentDesc = renderPass.stencilAttachment;
            stencilAttachmentDesc.clearStencil = renderPassDesc.depthAttachment.clearStencil;
            stencilAttachmentDesc.loadAction = AttachmentLoadActionToMTLLoadAction(renderPassDesc.depthAttachment.loadAction);
            stencilAttachmentDesc.storeAction = AttachmentStoreActionToMTLStoreAction(renderPassDesc.depthAttachment.storeAction);
            stencilAttachmentDesc.texture = renderPassDesc.depthAttachment.texture->GetRenderSurface();
            
            if (renderPassDesc.samples > 1)
                stencilAttachmentDesc.resolveTexture = renderPassDesc.depthAttachment.texture->GetHandle();
        }
    }
    
    mHandle->colorAttachments.resize(renderPassDesc.colorAttachments.size());
    for (uint32_t i = 0; i < renderPassDesc.colorAttachments.size(); i++)
    {
        const auto& colorAttachment = renderPassDesc.colorAttachments[i];
        mHandle->colorAttachments[i] = colorAttachment.texture->GetDescriptor();
        
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
        colorAttachmentDesc.texture = colorAttachment.texture->GetRenderSurface();
        
        if (renderPassDesc.samples > 1)
            colorAttachmentDesc.resolveTexture = colorAttachment.texture->GetHandle();
    }
    
    mHandle->renderCommandEncoder = [mHandle->commandBuffer renderCommandEncoderWithDescriptor:renderPass];
}

void CommandBuffer::EndRenderPass() const
{
    [mHandle->renderCommandEncoder endEncoding];
    mHandle->renderCommandEncoder = nil;
}

void CommandBuffer::BindGraphicsPipeline(const PipelineStateDescriptor& pipelineDesc, const RefCounted<Shader>& vertexShader, const RefCounted<Shader>& fragmentShader) const
{
    if (mHandle->hasDepthAttachment)
    {
        mHandle->pipelineState = MetalPipelineStateManager::GetGraphicsPipelineState(pipelineDesc, mHandle->colorAttachments, mHandle->depthAttachment, vertexShader, fragmentShader, mHandle->sampleCount);
        [mHandle->renderCommandEncoder setRenderPipelineState:mHandle->pipelineState.pipeline];
        [mHandle->renderCommandEncoder setDepthStencilState:mHandle->pipelineState.depthStencil];
    }
    else
    {
        mHandle->pipelineState = MetalPipelineStateManager::GetGraphicsPipelineState(pipelineDesc, mHandle->colorAttachments, vertexShader, fragmentShader, mHandle->sampleCount);
        [mHandle->renderCommandEncoder setRenderPipelineState:mHandle->pipelineState.pipeline];
    }
}

void CommandBuffer::SetViewport(const Size& size) const
{
    MTLViewport viewport{};
    viewport.width = size.width;
    viewport.height = size.height;
    viewport.zfar = 1.0f;
    [mHandle->renderCommandEncoder setViewport:viewport];
}

void CommandBuffer::SetVertexBuffer(const NativeGraphicsHandle buffer, BufferUsage usage, size_t size, size_t offset, uint32_t index) const
{
    [mHandle->renderCommandEncoder setVertexBuffer:buffer offset:offset atIndex:index];
}

void CommandBuffer::SetFragmentBuffer(const NativeGraphicsHandle buffer, BufferUsage usage, size_t size, size_t offset, uint32_t index) const
{
    [mHandle->renderCommandEncoder setFragmentBuffer:buffer offset:offset atIndex:index];
}

void CommandBuffer::SetVertexTexture(const NativeGraphicsHandle texture, uint32_t index) const
{
    [mHandle->renderCommandEncoder setVertexTexture:texture atIndex:index];
}

void CommandBuffer::SetFragmentTexture(const NativeGraphicsHandle texture, uint32_t index) const
{
    [mHandle->renderCommandEncoder setFragmentTexture:texture atIndex:index];
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

void CommandBuffer::CopyBuffer(const NativeGraphicsHandle src, const NativeGraphicsHandle dst, size_t size, uint32_t srcOffset, uint32_t dstOffset) const
{
    id<MTLBlitCommandEncoder> blitCommandEncoder = [mHandle->commandBuffer blitCommandEncoder];
    [blitCommandEncoder copyFromBuffer:src sourceOffset:srcOffset toBuffer:dst destinationOffset:dstOffset size:size];
    [blitCommandEncoder endEncoding];
}

void CommandBuffer::Blit(const RenderTexture& texture, const RenderTexture& target) const
{
    id<MTLBlitCommandEncoder> blitCommandEncoder = [mHandle->commandBuffer blitCommandEncoder];
    [blitCommandEncoder copyFromTexture:texture.GetHandle() toTexture:target.GetHandle()];
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
}

void CommandBuffer::Present() const
{
    MetalDevice::GetSwapchain().Present(mHandle->commandBuffer);
    Commit();
}

void CommandBuffer::WaitUntilCompleted() const
{
    [mHandle->commandBuffer waitUntilCompleted];
}

#endif
