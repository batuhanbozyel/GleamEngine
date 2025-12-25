#include "gpch.h"

#ifdef USE_METAL_RENDERER
#include "Renderer/CommandBuffer.h"

#include "MetalDevice.h"
#include "MetalUtils.h"

#include <metal_irconverter_runtime/metal_irconverter_runtime.h>

using namespace Gleam;

struct CommandBuffer::Impl
{
    MetalDevice* device = nullptr;
    
    id<MTLCommandBuffer> commandBuffer = nil;
    id<MTLRenderCommandEncoder> renderCommandEncoder = nil;
    id<MTLComputeCommandEncoder> computeCommandEncoder = nil;
    id<MetalPipeline> pipeline = nil;
    
    uint64_t topLevelArgumentBuffer[TopLevelArgumentBufferSize / sizeof(uint64_t)] = {};
};

CommandBuffer::CommandBuffer(GraphicsDevice* device)
    : mHandle(CreateScope<Impl>()), mDevice(device)
    , mConstantBuffer(device, 4194304) // 4 MB
{
    mHandle->device = static_cast<MetalDevice*>(device);
}

CommandBuffer::~CommandBuffer()
{
    WaitUntilCompleted();
}

void CommandBuffer::BeginRenderPass(const RenderPassDescriptor& renderPassDesc, const TStringView debugName) const
{
    [mHandle->device->GetResidencySet() commit];
    
    MTLRenderPassDescriptor* renderPass = [MTLRenderPassDescriptor renderPassDescriptor];
    if (renderPassDesc.depthAttachment.texture.IsValid())
    {
        const auto& depthAttachment = renderPassDesc.depthAttachment.texture.GetDescriptor();
        if (Utils::IsDepthFormat(depthAttachment.format))
        {
            MTLRenderPassDepthAttachmentDescriptor* depthAttachmentDesc = renderPass.depthAttachment;
            depthAttachmentDesc.clearDepth = renderPassDesc.depthAttachment.clearDepth;
            depthAttachmentDesc.loadAction = AttachmentLoadActionToMTLLoadAction(renderPassDesc.depthAttachment.loadAction);
            depthAttachmentDesc.storeAction = AttachmentStoreActionToMTLStoreAction(renderPassDesc.depthAttachment.storeAction);
            depthAttachmentDesc.texture = renderPassDesc.depthAttachment.texture.GetRenderTargetView();
        }
        
        if (Utils::IsDepthStencilFormat(depthAttachment.format))
        {
            MTLRenderPassStencilAttachmentDescriptor* stencilAttachmentDesc = renderPass.stencilAttachment;
            stencilAttachmentDesc.clearStencil = renderPassDesc.depthAttachment.clearStencil;
            stencilAttachmentDesc.loadAction = renderPass.depthAttachment.loadAction;
            stencilAttachmentDesc.storeAction = renderPass.depthAttachment.storeAction;
            stencilAttachmentDesc.texture = renderPass.depthAttachment.texture;
        }
    }
    
    for (uint32_t i = 0; i < renderPassDesc.colorAttachments.size(); i++)
    {
        const auto& colorAttachment = renderPassDesc.colorAttachments[i];
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
        colorAttachmentDesc.texture = colorAttachment.texture.GetRenderTargetView();
    }
    
    mHandle->renderCommandEncoder = [mHandle->commandBuffer renderCommandEncoderWithDescriptor:renderPass];
    mHandle->renderCommandEncoder.label = TO_NSSTRING(debugName.data());
}

void CommandBuffer::EndRenderPass() const
{
    [mHandle->renderCommandEncoder endEncoding];
    mHandle->renderCommandEncoder = nil;
}

void CommandBuffer::BeginComputePass(const TStringView debugName) const
{
    [mHandle->device->GetResidencySet() commit];
    
    mHandle->computeCommandEncoder = [mHandle->commandBuffer computeCommandEncoder];
    mHandle->computeCommandEncoder.label = TO_NSSTRING(debugName.data());
}

void CommandBuffer::EndComputePass() const
{
	[mHandle->computeCommandEncoder endEncoding];
    mHandle->computeCommandEncoder = nil;
}

void CommandBuffer::BindComputePipeline(const ComputePipeline& pipeline) const
{
    mHandle->pipeline = pipeline.GetHandle();

    id<MetalComputePipeline> computePipeline = (id<MetalComputePipeline>)mHandle->pipeline;
    [mHandle->computeCommandEncoder setComputePipelineState:computePipeline.pipelineState];

    // Descriptor heap
    [mHandle->computeCommandEncoder setBuffer:mHandle->device->GetSamplerHeap() offset:0 atIndex:kIRSamplerHeapBindPoint];
    [mHandle->computeCommandEncoder setBuffer:mHandle->device->GetCbvSrvUavHeap() offset:0 atIndex:kIRDescriptorHeapBindPoint];

    // Top-level argument buffer
    memset(mHandle->topLevelArgumentBuffer, 0, TopLevelArgumentBufferSize);
    
    // Sampler heap
    id<MTLBuffer> staticSamplers = mHandle->device->GetSamplerHeap();
    mHandle->topLevelArgumentBuffer[STATIC_SAMPLER_SLOT] = [staticSamplers gpuAddress];
}

void CommandBuffer::BindGraphicsPipeline(const GraphicsPipeline& pipeline) const
{
    mHandle->pipeline = pipeline.GetHandle();
    
    id<MetalGraphicsPipeline> renderPipeline = (id<MetalGraphicsPipeline>)mHandle->pipeline;
    [mHandle->renderCommandEncoder setRenderPipelineState:renderPipeline.pipelineState];
    if (renderPipeline.depthStencilState)
    {
        [mHandle->renderCommandEncoder setDepthStencilState:renderPipeline.depthStencilState];
    }
    
    [mHandle->renderCommandEncoder setCullMode:CullModeToMTLCullMode(pipeline.GetDescriptor().cullingMode)];
    [mHandle->renderCommandEncoder setTriangleFillMode:pipeline.GetDescriptor().wireframe ? MTLTriangleFillModeLines : MTLTriangleFillModeFill];
    
    // Descriptor heap
    [mHandle->renderCommandEncoder setVertexBuffer:mHandle->device->GetSamplerHeap() offset:0 atIndex:kIRSamplerHeapBindPoint];
    [mHandle->renderCommandEncoder setVertexBuffer:mHandle->device->GetCbvSrvUavHeap() offset:0 atIndex:kIRDescriptorHeapBindPoint];
    [mHandle->renderCommandEncoder setFragmentBuffer:mHandle->device->GetSamplerHeap() offset:0 atIndex:kIRSamplerHeapBindPoint];
    [mHandle->renderCommandEncoder setFragmentBuffer:mHandle->device->GetCbvSrvUavHeap() offset:0 atIndex:kIRDescriptorHeapBindPoint];
    
    // Top-level argument buffer
    memset(mHandle->topLevelArgumentBuffer, 0, TopLevelArgumentBufferSize);
    
    // Sampler heap
    id<MTLBuffer> staticSamplers = mHandle->device->GetSamplerHeap();
    mHandle->topLevelArgumentBuffer[STATIC_SAMPLER_SLOT] = [staticSamplers gpuAddress];
}

void CommandBuffer::SetViewport(const Size& size) const
{
    MTLViewport viewport{};
    viewport.width = size.width;
    viewport.height = size.height;
    viewport.zfar = 1.0f;
    [mHandle->renderCommandEncoder setViewport:viewport];
}

void CommandBuffer::SetScissorRect(const Rect& rect) const
{
	MTLScissorRect scissor{};
	scissor.x = static_cast<uint32_t>(rect.offset.x);
	scissor.y = static_cast<uint32_t>(rect.offset.y);
	scissor.width = static_cast<uint32_t>(rect.size.width);
	scissor.height = static_cast<uint32_t>(rect.size.height);
    [mHandle->renderCommandEncoder setScissorRect:scissor];
}

void CommandBuffer::SetConstantBuffer(const void* data, uint32_t size, uint32_t slot) const
{
    auto gpuAddress = [mConstantBuffer.GetHandle() gpuAddress]; 
	gpuAddress += mConstantBuffer.Write(data, size);
    mHandle->topLevelArgumentBuffer[slot] = gpuAddress;
}

void CommandBuffer::SetPushConstant(const void* data, uint32_t size) const
{
    memcpy(mHandle->topLevelArgumentBuffer + PUSH_CONSTANT_SLOT, data, size);
}

void CommandBuffer::Dispatch(uint32_t x, uint32_t y, uint32_t z) const
{
    id<MetalComputePipeline> pipeline = (id<MetalComputePipeline>)mHandle->pipeline;
    
    MTLSize threadGroupSize = MTLSizeMake(x, y, z);
    [mHandle->computeCommandEncoder setBytes:mHandle->topLevelArgumentBuffer length:TopLevelArgumentBufferSize atIndex:kIRArgumentBufferBindPoint];
    [mHandle->computeCommandEncoder dispatchThreadgroups:threadGroupSize threadsPerThreadgroup:pipeline.threadsPerThreadgroup];
}

void CommandBuffer::Draw(uint32_t vertexCount, uint32_t instanceCount) const
{
    [mHandle->renderCommandEncoder setVertexBytes:mHandle->topLevelArgumentBuffer length:TopLevelArgumentBufferSize atIndex:kIRArgumentBufferBindPoint];
    [mHandle->renderCommandEncoder setFragmentBytes:mHandle->topLevelArgumentBuffer length:TopLevelArgumentBufferSize atIndex:kIRArgumentBufferBindPoint];
    
    id<MetalGraphicsPipeline> pipeline = (id<MetalGraphicsPipeline>)mHandle->pipeline;
    IRRuntimeDrawPrimitives(mHandle->renderCommandEncoder, pipeline.topology, 0, vertexCount, instanceCount, 0);
}

void CommandBuffer::DrawIndexed(const Buffer& indexBuffer, IndexType type, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, uint32_t baseVertex) const
{
    [mHandle->renderCommandEncoder setVertexBytes:mHandle->topLevelArgumentBuffer length:TopLevelArgumentBufferSize atIndex:kIRArgumentBufferBindPoint];
    [mHandle->renderCommandEncoder setFragmentBytes:mHandle->topLevelArgumentBuffer length:TopLevelArgumentBufferSize atIndex:kIRArgumentBufferBindPoint];
    
    id<MetalGraphicsPipeline> pipeline = (id<MetalGraphicsPipeline>)mHandle->pipeline;
    MTLIndexType indexType = static_cast<MTLIndexType>(type);
    IRRuntimeDrawIndexedPrimitives(mHandle->renderCommandEncoder, pipeline.topology, indexCount, indexType, indexBuffer.GetHandle(), firstIndex * SizeOfIndexType(type), instanceCount);
}

void CommandBuffer::CopyBuffer(const NativeGraphicsHandle src, const NativeGraphicsHandle dst, size_t size, size_t srcOffset, size_t dstOffset) const
{
    id<MTLBlitCommandEncoder> blitCommandEncoder = [mHandle->commandBuffer blitCommandEncoder];
    [blitCommandEncoder setLabel:TO_NSSTRING("CommandBuffer::CopyBuffer")];
    [blitCommandEncoder copyFromBuffer:src sourceOffset:srcOffset toBuffer:dst destinationOffset:dstOffset size:size];
    [blitCommandEncoder endEncoding];
}

void CommandBuffer::Blit(const Texture& source, const Texture& destination) const
{
    id<MTLTexture> srcTexture = source.GetHandle();
    id<MTLTexture> dstTexture = destination.GetHandle();

    id<MTLBlitCommandEncoder> blitCommandEncoder = [mHandle->commandBuffer blitCommandEncoder];
    [blitCommandEncoder setLabel:TO_NSSTRING("CommandBuffer::Blit")];
    [blitCommandEncoder copyFromTexture:srcTexture toTexture:dstTexture];
    [blitCommandEncoder endEncoding];
}

void CommandBuffer::Barrier(const BarrierGroup& barrier) const
{
    // TODO: Implement Metal barriers when needed
}

void CommandBuffer::Begin(const TStringView debugName) const
{
    mHandle->commandBuffer = mHandle->device->AllocateCommandBuffer();
    mCommitted = false;
}

void CommandBuffer::End() const
{
    [mHandle->commandBuffer enqueue];
}

void CommandBuffer::Commit() const
{
    [mHandle->commandBuffer commit];
    mConstantBuffer.Reset();
    mCommitted = true;
}

void CommandBuffer::WaitUntilCompleted() const
{
    if (mCommitted)
	{
		[mHandle->commandBuffer waitUntilCompleted];
	}
    mCommitted = false;
}

NativeGraphicsHandle CommandBuffer::GetHandle() const
{
    return mHandle->commandBuffer;
}

#endif
