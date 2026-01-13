#include "gpch.h"

#ifdef USE_METAL_RENDERER
#include "Renderer/CommandBuffer.h"

#include "MetalDevice.h"
#include "MetalUtils.h"

#include <metal_irconverter_runtime/metal_irconverter_runtime.h>

using namespace Gleam;

static uint16_t IRMetalIndexToIRIndex(MTLIndexType indexType)
{
    return (uint16_t)(indexType+1);
}

struct CommandBuffer::Impl
{
    MetalDevice* device = nullptr;
    
    id<MTL4CommandBuffer> commandBuffer = nil;
    id<MTL4RenderCommandEncoder> renderCommandEncoder = nil;
    id<MTL4ComputeCommandEncoder> computeCommandEncoder = nil;
    id<MetalPipeline> pipeline = nil;
    
    id<MTLEvent> event = nil;
    uint64_t eventValue = 1;
    uint64_t waitEventValue = 0;
    
    MTLStages afterQueueStages = MTLStageAll;
    MTLStages beforeStages = MTLStageAll;
    
    uint64_t topLevelArgumentBuffer[TopLevelArgumentBufferSize / sizeof(uint64_t)] = {};
};

CommandBuffer::CommandBuffer(GraphicsDevice* device)
    : mHandle(CreateScope<Impl>()), mDevice(device)
    , mConstantBuffer(device, 4194304) // 4 MB
{
    mHandle->device = static_cast<MetalDevice*>(device);
    mHandle->event = [mHandle->device->GetHandle() newEvent];
}

CommandBuffer::~CommandBuffer()
{
    WaitUntilCompleted();
}

void CommandBuffer::BeginRenderPass(const RenderPassDescriptor& renderPassDesc, const TStringView debugName) const
{
    MTL4RenderPassDescriptor* renderPass = [MTL4RenderPassDescriptor new];
    if (renderPassDesc.depthAttachment.texture.IsValid())
    {
        const auto& depthAttachment = renderPassDesc.depthAttachment.texture.GetDescriptor();
        if (Utils::IsDepthFormat(depthAttachment.format))
        {
            MTLRenderPassDepthAttachmentDescriptor* depthAttachmentDesc = renderPass.depthAttachment;
            depthAttachmentDesc.clearDepth = renderPassDesc.depthAttachment.clearDepth;
            depthAttachmentDesc.loadAction = AttachmentLoadActionToMTLLoadAction(renderPassDesc.depthAttachment.loadAction);
            depthAttachmentDesc.storeAction = AttachmentStoreActionToMTLStoreAction(renderPassDesc.depthAttachment.storeAction);
            depthAttachmentDesc.texture = renderPassDesc.depthAttachment.texture.GetHandle();
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
        colorAttachmentDesc.texture = colorAttachment.texture.GetHandle();
    }
    
    mHandle->renderCommandEncoder = [mHandle->commandBuffer renderCommandEncoderWithDescriptor:renderPass];
    mHandle->renderCommandEncoder.label = TO_NSSTRING(debugName.data());
    
    [mHandle->renderCommandEncoder barrierAfterQueueStages:mHandle->afterQueueStages beforeStages:mHandle->beforeStages visibilityOptions:MTL4VisibilityOptionResourceAlias];
    [mHandle->renderCommandEncoder setArgumentTable:mHandle->device->GetArgumentTable() atStages:MTLRenderStageVertex | MTLRenderStageFragment];
}

void CommandBuffer::EndRenderPass() const
{
    [mHandle->renderCommandEncoder endEncoding];
    mHandle->renderCommandEncoder = nil;
}

void CommandBuffer::BeginComputePass(const TStringView debugName) const
{
    mHandle->computeCommandEncoder = [mHandle->commandBuffer computeCommandEncoder];
    mHandle->computeCommandEncoder.label = TO_NSSTRING(debugName.data());
    
    [mHandle->computeCommandEncoder barrierAfterQueueStages:mHandle->afterQueueStages beforeStages:mHandle->beforeStages visibilityOptions:MTL4VisibilityOptionResourceAlias];
    [mHandle->computeCommandEncoder setArgumentTable:mHandle->device->GetArgumentTable()];
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
    auto gpuAddress = [mConstantBuffer.GetHandle() gpuAddress];
    gpuAddress += mConstantBuffer.Write(mHandle->topLevelArgumentBuffer, TopLevelArgumentBufferSize);
    [mHandle->device->GetArgumentTable() setAddress:gpuAddress atIndex:kIRArgumentBufferBindPoint];
    
    id<MetalComputePipeline> pipeline = (id<MetalComputePipeline>)mHandle->pipeline;
    MTLSize threadGroupSize = MTLSizeMake(x, y, z);
    [mHandle->computeCommandEncoder dispatchThreadgroups:threadGroupSize threadsPerThreadgroup:pipeline.threadsPerThreadgroup];
}

void CommandBuffer::Draw(uint32_t vertexCount, uint32_t instanceCount) const
{
    auto gpuAddress = [mConstantBuffer.GetHandle() gpuAddress];
    
    IRRuntimeDrawArgument drawArgument = { .vertexCountPerInstance = vertexCount, .instanceCount = instanceCount, .startVertexLocation = 0, .startInstanceLocation = 0 };
    IRRuntimeDrawParams drawParams = { .draw = drawArgument };
    
    size_t drawOffset = mConstantBuffer.Write(drawParams);
    size_t nonIndexedDrawOffset = mConstantBuffer.Write(kIRNonIndexedDraw);
    size_t topLevelABOffset = mConstantBuffer.Write(mHandle->topLevelArgumentBuffer, TopLevelArgumentBufferSize);
    
    id<MTL4ArgumentTable> argumentTable = mHandle->device->GetArgumentTable();
    [argumentTable setAddress:(gpuAddress + drawOffset) atIndex:kIRArgumentBufferDrawArgumentsBindPoint];
    [argumentTable setAddress:(gpuAddress + nonIndexedDrawOffset) atIndex:kIRArgumentBufferUniformsBindPoint];
    [argumentTable setAddress:(gpuAddress + topLevelABOffset) atIndex:kIRArgumentBufferBindPoint];
    
    id<MetalGraphicsPipeline> pipeline = (id<MetalGraphicsPipeline>)mHandle->pipeline;
    [mHandle->renderCommandEncoder drawPrimitives:pipeline.topology vertexStart:0 vertexCount:vertexCount instanceCount:instanceCount baseInstance:0];
}

void CommandBuffer::DrawIndexed(const Buffer& indexBuffer, IndexType type, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, uint32_t baseVertex) const
{
    auto gpuAddress = [mConstantBuffer.GetHandle() gpuAddress];
    
    IRRuntimeDrawIndexedArgument drawArgument = { .indexCountPerInstance = indexCount, .instanceCount = instanceCount, .startIndexLocation = firstIndex * (uint32_t)SizeOfIndexType(type), .baseVertexLocation = (int)baseVertex, .startInstanceLocation = 0 };
    IRRuntimeDrawParams drawParams = { .drawIndexed = drawArgument };
    
    MTLIndexType indexType = static_cast<MTLIndexType>(type);
    uint16_t irIndexType = IRMetalIndexToIRIndex(indexType);
    
    size_t drawOffset = mConstantBuffer.Write(drawParams);
    size_t indexedDrawOffset = mConstantBuffer.Write(irIndexType);
    size_t topLevelABOffset = mConstantBuffer.Write(mHandle->topLevelArgumentBuffer, TopLevelArgumentBufferSize);
    
    id<MTL4ArgumentTable> argumentTable = mHandle->device->GetArgumentTable();
    [argumentTable setAddress:(gpuAddress + drawOffset) atIndex:kIRArgumentBufferDrawArgumentsBindPoint];
    [argumentTable setAddress:(gpuAddress + indexedDrawOffset) atIndex:kIRArgumentBufferUniformsBindPoint];
    [argumentTable setAddress:(gpuAddress + topLevelABOffset) atIndex:kIRArgumentBufferBindPoint];
    
    MTLGPUAddress indexBufferGpuAddress = [indexBuffer.GetHandle() gpuAddress] + drawArgument.startIndexLocation;
    size_t indexBufferLength = indexCount * SizeOfIndexType(type);
    
    id<MetalGraphicsPipeline> pipeline = (id<MetalGraphicsPipeline>)mHandle->pipeline;
    [mHandle->renderCommandEncoder drawIndexedPrimitives:pipeline.topology indexCount:indexCount indexType:indexType indexBuffer:indexBufferGpuAddress indexBufferLength:indexBufferLength instanceCount:instanceCount baseVertex:baseVertex baseInstance:0];
}

void CommandBuffer::CopyBuffer(const NativeGraphicsHandle src, const NativeGraphicsHandle dst, size_t size, size_t srcOffset, size_t dstOffset) const
{
    [mHandle->computeCommandEncoder setLabel:TO_NSSTRING("CommandBuffer::CopyBuffer")];
    [mHandle->computeCommandEncoder copyFromBuffer:src sourceOffset:srcOffset toBuffer:dst destinationOffset:dstOffset size:size];
    [mHandle->computeCommandEncoder endEncoding];
}

void CommandBuffer::Blit(const Texture& source, const Texture& destination) const
{
    id<MTLTexture> srcTexture = source.GetHandle();
    id<MTLTexture> dstTexture = destination.GetHandle();

    [mHandle->computeCommandEncoder setLabel:TO_NSSTRING("CommandBuffer::Blit")];
    [mHandle->computeCommandEncoder copyFromTexture:srcTexture toTexture:dstTexture];
    [mHandle->computeCommandEncoder endEncoding];
}

void CommandBuffer::Barrier(const BarrierGroup& barrier) const
{
    // TODO: Implement Metal barriers when needed
}

void CommandBuffer::Begin(const TStringView debugName) const
{
    mHandle->commandBuffer = mHandle->device->AllocateCommandBuffer();
    mHandle->commandBuffer.label = TO_NSSTRING(debugName.data());
    mCommitted = false;
}

void CommandBuffer::End() const
{
    [mHandle->commandBuffer endCommandBuffer];
}

void CommandBuffer::Commit() const
{
    mHandle->waitEventValue = mHandle->eventValue;
    
    id<MTL4CommandQueue> commandQueue = mHandle->device->GetCommandQueue();
    
    [mHandle->device->GetResidencySet() commit];
    [commandQueue commit:&mHandle->commandBuffer count:1u];
    [commandQueue signalEvent:mHandle->event value:mHandle->eventValue++];
    
    mConstantBuffer.Reset();
    mCommitted = true;
}

void CommandBuffer::WaitUntilCompleted() const
{
    if (mCommitted)
	{
        [mHandle->device->GetCommandQueue() waitForEvent:mHandle->event value:mHandle->waitEventValue];
	}
    mCommitted = false;
}

NativeGraphicsHandle CommandBuffer::GetHandle() const
{
    return mHandle->commandBuffer;
}

#endif
