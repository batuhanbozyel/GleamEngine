#include "gpch.h"

#ifdef USE_METAL_RENDERER
#include "Renderer/CommandBuffer.h"

#include "MetalDevice.h"
#include "MetalUtils.h"

#include <metal_irconverter_runtime/metal_irconverter_runtime.h>
#include <metal_irconverter_runtime/ir_raytracing.h>

using namespace Gleam;

static bool BarrierRequiresCacheFlush(BarrierAccess srcAccess, BarrierAccess dstAccess)
{
    bool srcIsWrite = (srcAccess == BarrierAccess::RenderTarget ||
                       srcAccess == BarrierAccess::UnorderedAccess ||
                       srcAccess == BarrierAccess::DepthStencilWrite ||
                       srcAccess == BarrierAccess::CopyDest);
    
    bool dstNeedsData = (dstAccess != BarrierAccess::None &&
                         dstAccess != BarrierAccess::Common);
    
    return srcIsWrite && dstNeedsData;
}

static uint16_t IRMetalIndexToIRIndex(MTLIndexType indexType)
{
    return (uint16_t)(indexType+1);
}

struct TopLevelArgumentBuffer
{
    uint64_t constantBuffers[PUSH_CONSTANT_SLOT] = {};
    uint32_t pushConstant[PUSH_CONSTANT_SIZE / sizeof(uint32_t)] = {};
    uint64_t samplerDescriptorHeap = 0;
};

struct CommandBuffer::Impl
{
    MetalDevice* device = nullptr;
    
    id<MTL4CommandBuffer> commandBuffer = nil;
    id<MTL4RenderCommandEncoder> renderCommandEncoder = nil;
    id<MTL4ComputeCommandEncoder> computeCommandEncoder = nil;
    id<MetalPipeline> pipeline = nil;
    
    id<MTLSharedEvent> event = nil;
    uint64_t eventValue = 0;
    
    TopLevelArgumentBuffer topLevelArgumentBuffer = {};
    
    struct ConsumerBarrier
    {
        MTLStages srcStages;
        MTLStages dstStages;
        MTL4VisibilityOptions visibility;
    };
    TArray<ConsumerBarrier> consumerBarriers;
    
    void FlushConsumerBarriers(id<MTL4CommandEncoder> encoder)
    {
        for (const auto& barrier : consumerBarriers)
        {
            [encoder barrierAfterQueueStages:barrier.srcStages
                                beforeStages:barrier.dstStages
                           visibilityOptions:barrier.visibility];
        }
        consumerBarriers.clear();
    }
};

CommandBuffer::CommandBuffer(GraphicsDevice* device)
    : mHandle(CreateScope<Impl>()), mDevice(device)
    , mConstantBuffer(device, 4194304) // 4 MB
{
    mHandle->device = static_cast<MetalDevice*>(device);
    mHandle->event = [mHandle->device->GetHandle() newSharedEvent];
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
    [mHandle->renderCommandEncoder setArgumentTable:mHandle->device->GetArgumentTable() atStages:MTLRenderStageVertex | MTLRenderStageFragment];
    
    mHandle->FlushConsumerBarriers(mHandle->renderCommandEncoder);
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
    [mHandle->computeCommandEncoder setArgumentTable:mHandle->device->GetArgumentTable()];
    
    mHandle->FlushConsumerBarriers(mHandle->computeCommandEncoder);
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
    memset(&mHandle->topLevelArgumentBuffer, 0, sizeof(TopLevelArgumentBuffer));
    mHandle->topLevelArgumentBuffer.samplerDescriptorHeap = [mHandle->device->GetSamplerHeap() gpuAddress];
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
    memset(&mHandle->topLevelArgumentBuffer, 0, sizeof(TopLevelArgumentBuffer));
    mHandle->topLevelArgumentBuffer.samplerDescriptorHeap = [mHandle->device->GetSamplerHeap() gpuAddress];
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
    mHandle->topLevelArgumentBuffer.constantBuffers[slot] = gpuAddress;
}

void CommandBuffer::SetPushConstant(const void* data, uint32_t size) const
{
    memcpy(mHandle->topLevelArgumentBuffer.pushConstant, data, size);
}

void CommandBuffer::Dispatch(uint32_t x, uint32_t y, uint32_t z) const
{
    auto gpuAddress = [mConstantBuffer.GetHandle() gpuAddress];
    size_t topLevelABOffset = mConstantBuffer.Write(mHandle->topLevelArgumentBuffer);
    
    id<MTL4ArgumentTable> argumentTable = mHandle->device->GetArgumentTable();
    [argumentTable setAddress:(gpuAddress + topLevelABOffset) atIndex:kIRArgumentBufferBindPoint];
    
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
    size_t topLevelABOffset = mConstantBuffer.Write(mHandle->topLevelArgumentBuffer);
    
    id<MTL4ArgumentTable> argumentTable = mHandle->device->GetArgumentTable();
    [argumentTable setAddress:(gpuAddress + drawOffset) atIndex:kIRArgumentBufferDrawArgumentsBindPoint];
    [argumentTable setAddress:(gpuAddress + nonIndexedDrawOffset) atIndex:kIRArgumentBufferUniformsBindPoint];
    [argumentTable setAddress:(gpuAddress + topLevelABOffset) atIndex:kIRArgumentBufferBindPoint];
    
    id<MetalGraphicsPipeline> pipeline = (id<MetalGraphicsPipeline>)mHandle->pipeline;
    [mHandle->renderCommandEncoder drawPrimitives:pipeline.topology vertexStart:0 vertexCount:vertexCount instanceCount:instanceCount baseInstance:0];
}

void CommandBuffer::DrawIndexed(const Buffer& indexBuffer, IndexType type, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex) const
{
    auto gpuAddress = [mConstantBuffer.GetHandle() gpuAddress];
    auto indexBufferOffset = firstIndex * (uint32_t)SizeOfIndexType(type);
    
    IRRuntimeDrawIndexedArgument drawArgument = { .indexCountPerInstance = indexCount, .instanceCount = instanceCount, .startIndexLocation = indexBufferOffset, .baseVertexLocation = 0, .startInstanceLocation = 0 };
    IRRuntimeDrawParams drawParams = { .drawIndexed = drawArgument };
    
    MTLIndexType indexType = static_cast<MTLIndexType>(type);
    uint16_t irIndexType = IRMetalIndexToIRIndex(indexType);
    
    size_t drawOffset = mConstantBuffer.Write(drawParams);
    size_t indexedDrawOffset = mConstantBuffer.Write(irIndexType);
    size_t topLevelABOffset = mConstantBuffer.Write(mHandle->topLevelArgumentBuffer);
    
    id<MTL4ArgumentTable> argumentTable = mHandle->device->GetArgumentTable();
    [argumentTable setAddress:(gpuAddress + drawOffset) atIndex:kIRArgumentBufferDrawArgumentsBindPoint];
    [argumentTable setAddress:(gpuAddress + indexedDrawOffset) atIndex:kIRArgumentBufferUniformsBindPoint];
    [argumentTable setAddress:(gpuAddress + topLevelABOffset) atIndex:kIRArgumentBufferBindPoint];
    
    MTLGPUAddress indexBufferGpuAddress = [indexBuffer.GetHandle() gpuAddress] + indexBufferOffset;
    size_t indexBufferLength = indexBuffer.GetSize();
    
    id<MetalGraphicsPipeline> pipeline = (id<MetalGraphicsPipeline>)mHandle->pipeline;
    [mHandle->renderCommandEncoder drawIndexedPrimitives:pipeline.topology indexCount:indexCount indexType:indexType indexBuffer:indexBufferGpuAddress indexBufferLength:indexBufferLength instanceCount:instanceCount baseVertex:0 baseInstance:0];
}

void CommandBuffer::CopyBuffer(const NativeGraphicsHandle src, const NativeGraphicsHandle dst, size_t size, size_t srcOffset, size_t dstOffset) const
{
    [mHandle->computeCommandEncoder setLabel:TO_NSSTRING("CommandBuffer::CopyBuffer")];
    [mHandle->computeCommandEncoder copyFromBuffer:src sourceOffset:srcOffset toBuffer:dst destinationOffset:dstOffset size:size];
}

void CommandBuffer::Blit(const Texture& source, const Texture& destination) const
{
    id<MTLTexture> srcTexture = source.GetHandle();
    id<MTLTexture> dstTexture = destination.GetHandle();

    [mHandle->computeCommandEncoder setLabel:TO_NSSTRING("CommandBuffer::Blit")];
    [mHandle->computeCommandEncoder copyFromTexture:srcTexture toTexture:dstTexture];
}

void CommandBuffer::Barrier(const BarrierGroup& barrier) const
{
    if (barrier.bufferBarriers.empty() && barrier.textureBarriers.empty())
    {
        return;
    }
    
    MTLStages allSrcStages = 0;
    MTLStages allDstStages = 0;
    bool needsCacheFlush = false;
    
    for (const auto& bufferBarrier : barrier.bufferBarriers)
    {
        if (bufferBarrier.srcStage == BarrierStage::None)
        {
            continue;
        }
        
        allSrcStages |= BarrierStageToMTLStages(bufferBarrier.srcStage);
        allDstStages |= BarrierStageToMTLStages(bufferBarrier.dstStage);
        needsCacheFlush |= BarrierRequiresCacheFlush(bufferBarrier.srcAccess, bufferBarrier.dstAccess);
    }
    
    for (const auto& textureBarrier : barrier.textureBarriers)
    {
        if (textureBarrier.srcStage == BarrierStage::None)
        {
            continue;
        }
        
        allSrcStages |= BarrierStageToMTLStages(textureBarrier.srcStage);
        allDstStages |= BarrierStageToMTLStages(textureBarrier.dstStage);
        needsCacheFlush |= BarrierRequiresCacheFlush(textureBarrier.srcAccess, textureBarrier.dstAccess);
    }
    
    if (allSrcStages == 0)
    {
        return;
    }
    
    MTL4VisibilityOptions visibility = needsCacheFlush ? MTL4VisibilityOptionDevice : MTL4VisibilityOptionNone;
    if (mHandle->renderCommandEncoder)
    {
        MTLStages validRenderStages = MTLStageVertex | MTLStageFragment;
        MTLStages renderSrcStages = allSrcStages & validRenderStages;
        MTLStages renderDstStages = allDstStages & validRenderStages;
        
        if (renderSrcStages != 0 && renderDstStages != 0)
        {
            [mHandle->renderCommandEncoder barrierAfterEncoderStages:renderSrcStages
                                                  beforeEncoderStages:renderDstStages
                                                    visibilityOptions:visibility];
        }
        
        MTLStages nonRenderSrcStages = allSrcStages & ~validRenderStages;
        MTLStages nonRenderDstStages = allDstStages & ~validRenderStages;
        
        if (nonRenderSrcStages != 0 || nonRenderDstStages != 0)
        {
            mHandle->consumerBarriers.push_back({nonRenderSrcStages, nonRenderDstStages, visibility });
        }
    }
    else if (mHandle->computeCommandEncoder)
    {
        MTLStages validComputeStages = MTLStageDispatch | MTLStageBlit | MTLStageAccelerationStructure;
        MTLStages computeSrcStages = allSrcStages & validComputeStages;
        MTLStages computeDstStages = allDstStages & validComputeStages;
        
        if (computeSrcStages != 0 && computeDstStages != 0)
        {
            [mHandle->computeCommandEncoder barrierAfterEncoderStages:computeSrcStages
                                                   beforeEncoderStages:computeDstStages
                                                     visibilityOptions:visibility];
        }
        
        MTLStages nonComputeSrcStages = allSrcStages & ~validComputeStages;
        MTLStages nonComputeDstStages = allDstStages & ~validComputeStages;
        if (nonComputeSrcStages != 0 || nonComputeDstStages != 0)
        {
            mHandle->consumerBarriers.push_back({ nonComputeSrcStages, nonComputeDstStages, visibility });
        }
    }
    else
    {
        mHandle->consumerBarriers.push_back({ allSrcStages, allDstStages, visibility });
    }
}

void CommandBuffer::Begin(const TStringView debugName) const
{
    mHandle->commandBuffer = mHandle->device->AllocateCommandBuffer();
    mHandle->commandBuffer.label = TO_NSSTRING(debugName.data());
    mHandle->event.label = mHandle->commandBuffer.label;
    mCommitted = false;
}

void CommandBuffer::End() const
{
    [mHandle->commandBuffer endCommandBuffer];
}

void CommandBuffer::Commit() const
{
    id<MTL4CommandQueue> commandQueue = mHandle->device->GetCommandQueue();
    
    [mHandle->device->GetResidencySet() commit];
    [commandQueue commit:&mHandle->commandBuffer count:1u];
    [commandQueue signalEvent:mHandle->event value:++mHandle->eventValue];
    
    mConstantBuffer.Reset();
    mCommitted = true;
}

void CommandBuffer::WaitUntilCompleted() const
{
    if (mCommitted)
	{
        WaitForMTLSharedEvent(mHandle->event, mHandle->eventValue);
	}
    mCommitted = false;
}

NativeGraphicsHandle CommandBuffer::GetHandle() const
{
    return mHandle->commandBuffer;
}

#endif
