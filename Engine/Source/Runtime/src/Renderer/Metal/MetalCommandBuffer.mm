#include "gpch.h"

#ifdef USE_METAL_RENDERER
#include "Renderer/CommandBuffer.h"

#include "MetalDevice.h"
#include "MetalUtils.h"

#include <metal_irconverter_runtime/metal_irconverter_runtime.h>
#include <metal_irconverter_runtime/ir_raytracing.h>

using namespace Gleam;

static_assert(sizeof(DrawIndirectArguments) == sizeof(IRRuntimeDrawArgument));
static_assert(sizeof(DrawIndexedIndirectArguments) == sizeof(IRRuntimeDrawIndexedArgument));
static_assert(sizeof(DispatchIndirectArguments) == sizeof(MTLDispatchThreadgroupsIndirectArguments));

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
    MetalCommandPool* commandPool = nullptr;

    id<MTL4CommandBuffer> commandBuffer = nil;
    id<MTL4RenderCommandEncoder> renderCommandEncoder = nil;
    id<MTL4ComputeCommandEncoder> computeCommandEncoder = nil;

    id<MTLSharedEvent> event = nil;
    uint64_t eventValue = 0;
    
    TopLevelArgumentBuffer topLevelArgumentBuffer = {};
    PipelineHandle pipeline;

    NativePipelineHandle transformDispatchRaysIndirectArgsPipeline = 0;

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

CommandBuffer::CommandBuffer(GraphicsDevice* device, GPUAllocator* transientAllocator)
    : mHandle(CreateScope<Impl>()), mDevice(device)
    , mTransientAllocator(transientAllocator)
    , mConstantBuffer(device, 4194304) // 4 MB
{
    mHandle->device = static_cast<MetalDevice*>(device);
    mHandle->event = [mHandle->device->GetHandle() newSharedEvent];
    mHandle->transformDispatchRaysIndirectArgsPipeline = mHandle->device->CompileNativeComputePipeline("transformDispatchRaysIndirectArgs");
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
    [mHandle->renderCommandEncoder setArgumentTable:mHandle->device->GetArgumentTable() atStages:MTLRenderStageVertex | MTLRenderStageFragment | MTLRenderStageObject | MTLRenderStageMesh];
    
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
    mHandle->pipeline = pipeline.GetHash();

    id<MetalComputePipeline> computePipeline = (id<MetalComputePipeline>)pipeline.GetHandle();
    [mHandle->computeCommandEncoder setComputePipelineState:computePipeline.pipelineState];

    // Top-level argument buffer
    memset(&mHandle->topLevelArgumentBuffer, 0, sizeof(TopLevelArgumentBuffer));
    mHandle->topLevelArgumentBuffer.samplerDescriptorHeap = [mHandle->device->GetSamplerHeap() gpuAddress];
}

void CommandBuffer::BindGraphicsPipeline(const GraphicsPipeline& pipeline) const
{
    mHandle->pipeline = pipeline.GetHash();
    
    id<MetalGraphicsPipeline> renderPipeline = (id<MetalGraphicsPipeline>)pipeline.GetHandle();
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

void CommandBuffer::BindRayTracingPipeline(const RayTracingPipeline& pipeline) const
{
    mHandle->pipeline = pipeline.GetHash();

    id<MetalRayTracingPipeline> rayTracingPipeline = (id<MetalRayTracingPipeline>)pipeline.GetHandle();
    [mHandle->computeCommandEncoder setComputePipelineState:rayTracingPipeline.pipelineState];

    // Top-level argument buffer
    memset(&mHandle->topLevelArgumentBuffer, 0, sizeof(TopLevelArgumentBuffer));
    mHandle->topLevelArgumentBuffer.samplerDescriptorHeap = [mHandle->device->GetSamplerHeap() gpuAddress];
}

void CommandBuffer::BindMeshPipeline(const MeshPipeline& pipeline) const
{
    mHandle->pipeline = pipeline.GetHash();

    id<MetalMeshPipeline> meshPipeline = (id<MetalMeshPipeline>)pipeline.GetHandle();
    [mHandle->renderCommandEncoder setRenderPipelineState:meshPipeline.pipelineState];
    if (meshPipeline.depthStencilState)
    {
        [mHandle->renderCommandEncoder setDepthStencilState:meshPipeline.depthStencilState];
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

void CommandBuffer::DispatchRays(uint32_t width, uint32_t height, uint32_t depth) const
{
    const auto& pipeline = static_cast<RayTracingPipelineHandle>(mHandle->pipeline).GetPipeline();
    const auto& sbt = pipeline.GetShaderBindingTable();
    
    auto gpuAddress = [mConstantBuffer.GetHandle() gpuAddress];
    size_t topLevelABOffset = mConstantBuffer.Write(mHandle->topLevelArgumentBuffer);
    
    IRDispatchRaysArgument dispatchArg = {};
    dispatchArg.DispatchRaysDesc.RayGenerationShaderRecord.StartAddress = sbt.GetRayGenRecord().startAddress;
    dispatchArg.DispatchRaysDesc.RayGenerationShaderRecord.SizeInBytes  = sbt.GetRayGenRecord().sizeInBytes;
    
    dispatchArg.DispatchRaysDesc.MissShaderTable.StartAddress  = sbt.GetMissRecord().startAddress;
    dispatchArg.DispatchRaysDesc.MissShaderTable.SizeInBytes   = sbt.GetMissRecord().sizeInBytes;
    dispatchArg.DispatchRaysDesc.MissShaderTable.StrideInBytes = sbt.GetMissRecord().strideInBytes;
              
    dispatchArg.DispatchRaysDesc.HitGroupTable.StartAddress  = sbt.GetHitGroupRecord().startAddress;
    dispatchArg.DispatchRaysDesc.HitGroupTable.SizeInBytes   = sbt.GetHitGroupRecord().sizeInBytes;
    dispatchArg.DispatchRaysDesc.HitGroupTable.StrideInBytes = sbt.GetHitGroupRecord().strideInBytes;
    
    dispatchArg.DispatchRaysDesc.Width  = width;
    dispatchArg.DispatchRaysDesc.Height = height;
    dispatchArg.DispatchRaysDesc.Depth  = depth;
    
    dispatchArg.GRS         = gpuAddress + topLevelABOffset;
    dispatchArg.ResDescHeap = [mHandle->device->GetCbvSrvUavHeap() gpuAddress];
    dispatchArg.SmpDescHeap = [mHandle->device->GetSamplerHeap() gpuAddress];
    
    id<MetalRayTracingPipeline> rayTracingPipeline = (id<MetalRayTracingPipeline>)pipeline.GetHandle();
    dispatchArg.VisibleFunctionTable = [rayTracingPipeline.visibleFunctionTable gpuResourceID];
    if (rayTracingPipeline.intersectionFunctionTable)
    {
      dispatchArg.IntersectionFunctionTable = [rayTracingPipeline.intersectionFunctionTable gpuResourceID];
    }
    size_t dispatchArgOffset = mConstantBuffer.Write(dispatchArg);
    
    id<MTL4ArgumentTable> argumentTable = mHandle->device->GetArgumentTable();
    [argumentTable setAddress:(gpuAddress + topLevelABOffset) atIndex:kIRArgumentBufferBindPoint];
    [argumentTable setAddress:(gpuAddress + dispatchArgOffset) atIndex:kIRRayDispatchArgumentsBindPoint];
    
    constexpr uint32_t tgX = 8, tgY = 8;
    MTLSize threadgroups = MTLSizeMake((width + tgX - 1) / tgX, (height + tgY - 1) / tgY, depth);
    MTLSize threadsPerGroup = MTLSizeMake(tgX, tgY, 1);
    [mHandle->computeCommandEncoder dispatchThreadgroups:threadgroups threadsPerThreadgroup:threadsPerGroup];
}

void CommandBuffer::DispatchRaysIndirect(const Buffer& argsBuffer, size_t offset) const
{
    const auto& pipeline = static_cast<RayTracingPipelineHandle>(mHandle->pipeline).GetPipeline();
    const auto& sbt = pipeline.GetShaderBindingTable();
    id<MetalRayTracingPipeline> rayTracingPipeline = (id<MetalRayTracingPipeline>)pipeline.GetHandle();
    id<MTL4ArgumentTable> argumentTable = mHandle->device->GetArgumentTable();
    auto gpuAddress = [mConstantBuffer.GetHandle() gpuAddress];

    BufferDescriptor scratchDesc;
    scratchDesc.name = "DispatchRaysIndirect Threadgroups";
    scratchDesc.size = 3 * sizeof(uint32_t);
    Buffer scratch = mDevice->CreateBuffer(mTransientAllocator, scratchDesc);

    constexpr uint32_t tgX = 8, tgY = 8;
    struct { uint32_t x, y; } groupSize = { tgX, tgY };
    size_t groupSizeOffset = mConstantBuffer.Write(groupSize);

    id<MTLComputePipelineState> transformPipeline = mHandle->device->GetNativeComputePipeline(mHandle->transformDispatchRaysIndirectArgsPipeline);
    [mHandle->computeCommandEncoder setComputePipelineState:transformPipeline];
    [argumentTable setAddress:([argsBuffer.GetHandle() gpuAddress] + offset) atIndex:13];
    [argumentTable setAddress:[scratch.GetHandle() gpuAddress] atIndex:14];
    [argumentTable setAddress:(gpuAddress + groupSizeOffset) atIndex:15];
    [mHandle->computeCommandEncoder dispatchThreadgroups:MTLSizeMake(1, 1, 1) threadsPerThreadgroup:MTLSizeMake(1, 1, 1)];

    [mHandle->computeCommandEncoder barrierAfterEncoderStages:MTLStageDispatch
                                          beforeEncoderStages:MTLStageDispatch
                                            visibilityOptions:MTL4VisibilityOptionDevice];

    [mHandle->computeCommandEncoder setComputePipelineState:rayTracingPipeline.pipelineState];

    size_t topLevelABOffset = mConstantBuffer.Write(mHandle->topLevelArgumentBuffer);

    IRDispatchRaysArgument dispatchArg = {};
    dispatchArg.DispatchRaysDesc.RayGenerationShaderRecord.StartAddress = sbt.GetRayGenRecord().startAddress;
    dispatchArg.DispatchRaysDesc.RayGenerationShaderRecord.SizeInBytes  = sbt.GetRayGenRecord().sizeInBytes;

    dispatchArg.DispatchRaysDesc.MissShaderTable.StartAddress  = sbt.GetMissRecord().startAddress;
    dispatchArg.DispatchRaysDesc.MissShaderTable.SizeInBytes   = sbt.GetMissRecord().sizeInBytes;
    dispatchArg.DispatchRaysDesc.MissShaderTable.StrideInBytes = sbt.GetMissRecord().strideInBytes;

    dispatchArg.DispatchRaysDesc.HitGroupTable.StartAddress  = sbt.GetHitGroupRecord().startAddress;
    dispatchArg.DispatchRaysDesc.HitGroupTable.SizeInBytes   = sbt.GetHitGroupRecord().sizeInBytes;
    dispatchArg.DispatchRaysDesc.HitGroupTable.StrideInBytes = sbt.GetHitGroupRecord().strideInBytes;

    dispatchArg.GRS         = gpuAddress + topLevelABOffset;
    dispatchArg.ResDescHeap = [mHandle->device->GetCbvSrvUavHeap() gpuAddress];
    dispatchArg.SmpDescHeap = [mHandle->device->GetSamplerHeap() gpuAddress];
    dispatchArg.VisibleFunctionTable = [rayTracingPipeline.visibleFunctionTable gpuResourceID];
    if (rayTracingPipeline.intersectionFunctionTable)
    {
      dispatchArg.IntersectionFunctionTable = [rayTracingPipeline.intersectionFunctionTable gpuResourceID];
    }
    size_t dispatchArgOffset = mConstantBuffer.Write(dispatchArg);

    [argumentTable setAddress:(gpuAddress + topLevelABOffset) atIndex:kIRArgumentBufferBindPoint];
    [argumentTable setAddress:(gpuAddress + dispatchArgOffset) atIndex:kIRRayDispatchArgumentsBindPoint];

    [mHandle->computeCommandEncoder dispatchThreadgroupsWithIndirectBuffer:[scratch.GetHandle() gpuAddress]
                                                    threadsPerThreadgroup:MTLSizeMake(tgX, tgY, 1)];

    mDevice->Dispose(mTransientAllocator, scratch, BarrierStage::ExecuteIndirect);
}

void CommandBuffer::DispatchMesh(uint32_t x, uint32_t y, uint32_t z) const
{
    const auto& pipeline = static_cast<MeshPipelineHandle>(mHandle->pipeline).GetPipeline();
    id<MetalMeshPipeline> meshPipeline = (id<MetalMeshPipeline>)pipeline.GetHandle();

    auto gpuAddress = [mConstantBuffer.GetHandle() gpuAddress];
    size_t topLevelABOffset = mConstantBuffer.Write(mHandle->topLevelArgumentBuffer);

    id<MTL4ArgumentTable> argumentTable = mHandle->device->GetArgumentTable();
    [argumentTable setAddress:(gpuAddress + topLevelABOffset) atIndex:kIRArgumentBufferBindPoint];

    [mHandle->renderCommandEncoder drawMeshThreadgroups:MTLSizeMake(x, y, z)
                          threadsPerObjectThreadgroup:meshPipeline.objectThreadsPerThreadgroup
                            threadsPerMeshThreadgroup:meshPipeline.meshThreadsPerThreadgroup];
}

void CommandBuffer::Dispatch(uint32_t x, uint32_t y, uint32_t z) const
{
    const auto& pipeline = static_cast<ComputePipelineHandle>(mHandle->pipeline).GetPipeline();
    id<MetalComputePipeline> computePipeline = (id<MetalComputePipeline>)pipeline.GetHandle();
    
    auto gpuAddress = [mConstantBuffer.GetHandle() gpuAddress];
    size_t topLevelABOffset = mConstantBuffer.Write(mHandle->topLevelArgumentBuffer);
    
    id<MTL4ArgumentTable> argumentTable = mHandle->device->GetArgumentTable();
    [argumentTable setAddress:(gpuAddress + topLevelABOffset) atIndex:kIRArgumentBufferBindPoint];
    
    MTLSize threadGroupSize = MTLSizeMake(x, y, z);
    [mHandle->computeCommandEncoder dispatchThreadgroups:threadGroupSize threadsPerThreadgroup:computePipeline.threadsPerThreadgroup];
}

void CommandBuffer::Draw(uint32_t vertexCount, uint32_t instanceCount) const
{
    const auto& pipeline = static_cast<GraphicsPipelineHandle>(mHandle->pipeline).GetPipeline();
    id<MetalGraphicsPipeline> renderPipeline = (id<MetalGraphicsPipeline>)pipeline.GetHandle();
    
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
    
    [mHandle->renderCommandEncoder drawPrimitives:renderPipeline.topology vertexStart:0 vertexCount:vertexCount instanceCount:instanceCount baseInstance:0];
}

void CommandBuffer::DrawIndexed(const Buffer& indexBuffer, IndexType type, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex) const
{
    const auto& pipeline = static_cast<GraphicsPipelineHandle>(mHandle->pipeline).GetPipeline();
    id<MetalGraphicsPipeline> renderPipeline = (id<MetalGraphicsPipeline>)pipeline.GetHandle();
    
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
    
    [mHandle->renderCommandEncoder drawIndexedPrimitives:renderPipeline.topology indexCount:indexCount indexType:indexType indexBuffer:indexBufferGpuAddress indexBufferLength:indexBufferLength instanceCount:instanceCount baseVertex:0 baseInstance:0];
}

void CommandBuffer::DrawIndirect(const Buffer& argsBuffer, size_t offset) const
{
    const auto& pipeline = static_cast<GraphicsPipelineHandle>(mHandle->pipeline).GetPipeline();
    id<MetalGraphicsPipeline> renderPipeline = (id<MetalGraphicsPipeline>)pipeline.GetHandle();

    auto gpuAddress = [mConstantBuffer.GetHandle() gpuAddress];
    MTLGPUAddress argsAddress = [argsBuffer.GetHandle() gpuAddress] + offset;

    size_t nonIndexedDrawOffset = mConstantBuffer.Write(kIRNonIndexedDraw);
    size_t topLevelABOffset = mConstantBuffer.Write(mHandle->topLevelArgumentBuffer);

    id<MTL4ArgumentTable> argumentTable = mHandle->device->GetArgumentTable();
    [argumentTable setAddress:argsAddress atIndex:kIRArgumentBufferDrawArgumentsBindPoint];
    [argumentTable setAddress:(gpuAddress + nonIndexedDrawOffset) atIndex:kIRArgumentBufferUniformsBindPoint];
    [argumentTable setAddress:(gpuAddress + topLevelABOffset) atIndex:kIRArgumentBufferBindPoint];

    [mHandle->renderCommandEncoder drawPrimitives:renderPipeline.topology indirectBuffer:argsAddress];
}

void CommandBuffer::DrawIndexedIndirect(const Buffer& indexBuffer, IndexType type, const Buffer& argsBuffer, size_t offset) const
{
    const auto& pipeline = static_cast<GraphicsPipelineHandle>(mHandle->pipeline).GetPipeline();
    id<MetalGraphicsPipeline> renderPipeline = (id<MetalGraphicsPipeline>)pipeline.GetHandle();

    auto gpuAddress = [mConstantBuffer.GetHandle() gpuAddress];
    MTLGPUAddress argsAddress = [argsBuffer.GetHandle() gpuAddress] + offset;

    MTLIndexType indexType = static_cast<MTLIndexType>(type);
    uint16_t irIndexType = IRMetalIndexToIRIndex(indexType);

    size_t indexedDrawOffset = mConstantBuffer.Write(irIndexType);
    size_t topLevelABOffset = mConstantBuffer.Write(mHandle->topLevelArgumentBuffer);

    id<MTL4ArgumentTable> argumentTable = mHandle->device->GetArgumentTable();
    [argumentTable setAddress:argsAddress atIndex:kIRArgumentBufferDrawArgumentsBindPoint];
    [argumentTable setAddress:(gpuAddress + indexedDrawOffset) atIndex:kIRArgumentBufferUniformsBindPoint];
    [argumentTable setAddress:(gpuAddress + topLevelABOffset) atIndex:kIRArgumentBufferBindPoint];

    MTLGPUAddress indexBufferGpuAddress = [indexBuffer.GetHandle() gpuAddress];
    size_t indexBufferLength = indexBuffer.GetSize();

    [mHandle->renderCommandEncoder drawIndexedPrimitives:renderPipeline.topology indexType:indexType indexBuffer:indexBufferGpuAddress indexBufferLength:indexBufferLength indirectBuffer:argsAddress];
}

void CommandBuffer::DispatchIndirect(const Buffer& argsBuffer, size_t offset) const
{
    const auto& pipeline = static_cast<ComputePipelineHandle>(mHandle->pipeline).GetPipeline();
    id<MetalComputePipeline> computePipeline = (id<MetalComputePipeline>)pipeline.GetHandle();

    auto gpuAddress = [mConstantBuffer.GetHandle() gpuAddress];
    size_t topLevelABOffset = mConstantBuffer.Write(mHandle->topLevelArgumentBuffer);

    id<MTL4ArgumentTable> argumentTable = mHandle->device->GetArgumentTable();
    [argumentTable setAddress:(gpuAddress + topLevelABOffset) atIndex:kIRArgumentBufferBindPoint];

    MTLGPUAddress argsAddress = [argsBuffer.GetHandle() gpuAddress] + offset;
    [mHandle->computeCommandEncoder dispatchThreadgroupsWithIndirectBuffer:argsAddress threadsPerThreadgroup:computePipeline.threadsPerThreadgroup];
}

void CommandBuffer::DispatchMeshIndirect(const Buffer& argsBuffer, size_t offset) const
{
    const auto& pipeline = static_cast<MeshPipelineHandle>(mHandle->pipeline).GetPipeline();
    id<MetalMeshPipeline> meshPipeline = (id<MetalMeshPipeline>)pipeline.GetHandle();

    auto gpuAddress = [mConstantBuffer.GetHandle() gpuAddress];
    size_t topLevelABOffset = mConstantBuffer.Write(mHandle->topLevelArgumentBuffer);

    id<MTL4ArgumentTable> argumentTable = mHandle->device->GetArgumentTable();
    [argumentTable setAddress:(gpuAddress + topLevelABOffset) atIndex:kIRArgumentBufferBindPoint];

    MTLGPUAddress argsAddress = [argsBuffer.GetHandle() gpuAddress] + offset;
    [mHandle->renderCommandEncoder drawMeshThreadgroupsWithIndirectBuffer:argsAddress
                          threadsPerObjectThreadgroup:meshPipeline.objectThreadsPerThreadgroup
                            threadsPerMeshThreadgroup:meshPipeline.meshThreadsPerThreadgroup];
}

void CommandBuffer::CopyBuffer(const NativeGraphicsHandle src, const NativeGraphicsHandle dst, size_t size, size_t srcOffset, size_t dstOffset) const
{
    [mHandle->computeCommandEncoder setLabel:TO_NSSTRING("CommandBuffer::CopyBuffer")];
    [mHandle->computeCommandEncoder copyFromBuffer:src sourceOffset:srcOffset toBuffer:dst destinationOffset:dstOffset size:size];
}

void CommandBuffer::ClearBuffer(const Buffer& buffer, uint8_t value) const
{
    [mHandle->computeCommandEncoder fillBuffer:buffer.GetHandle() range:NSMakeRange(0, buffer.GetSize()) value:value];
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
    
    if (allSrcStages == 0 || allDstStages == 0)
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
        
        if (nonRenderSrcStages != 0 && nonRenderDstStages != 0)
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
        if (nonComputeSrcStages != 0 && nonComputeDstStages != 0)
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
    mHandle->commandPool = mHandle->device->GetCommandQueue().AcquirePool();
    mHandle->commandBuffer = mHandle->commandPool->AllocateCommandBuffer(debugName);
    mHandle->event.label = mHandle->commandBuffer.label;
    mCommitted = false;
}

void CommandBuffer::End() const
{
    [mHandle->commandBuffer endCommandBuffer];
}

void CommandBuffer::Commit() const
{
    id<MTL4CommandBuffer> commandBuffer = mHandle->commandBuffer;
    
    [mHandle->device->GetResidencySet() commit];
    mHandle->device->GetCommandQueue().Commit(&commandBuffer, 1);
    mHandle->device->GetCommandQueue().Signal(mHandle->event, ++mHandle->eventValue);

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
