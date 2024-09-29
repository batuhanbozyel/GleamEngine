#include "gpch.h"

#ifdef USE_METAL_RENDERER
#include "Renderer/CommandBuffer.h"
#include "MetalPipelineStateManager.h"
#include "Core/Application.h"

#include <metal_irconverter_runtime/metal_irconverter_runtime.h>

using namespace Gleam;

struct CommandBuffer::Impl
{
    MetalDevice* device = nullptr;
    
    id<MTLCommandBuffer> commandBuffer = nil;
    id<MTLRenderCommandEncoder> renderCommandEncoder = nil;
    const MetalPipeline* pipeline = nullptr;
    
    Buffer topLevelArgumentBuffer;
    TArray<Buffer, PUSH_CONSTANT_SLOT> constantBuffers;
    
    TArray<TextureDescriptor> colorAttachments;
    TextureDescriptor depthAttachment;
    bool hasDepthAttachment = false;
    uint32_t sampleCount = 1;
};

CommandBuffer::CommandBuffer(GraphicsDevice* device)
    : mHandle(CreateScope<Impl>()), mDevice(device)
{
    mHandle->device = static_cast<MetalDevice*>(device);
    
    HeapDescriptor descriptor;
    descriptor.name = "CommandBuffer::StagingHeap";
    descriptor.size = 4194304; // 4 MB;
    descriptor.memoryType = MemoryType::CPU;
    mStagingHeap = mDevice->CreateHeap(descriptor);
}

CommandBuffer::~CommandBuffer()
{
    if (mHandle->topLevelArgumentBuffer.IsValid())
    {
        mDevice->Dispose(mHandle->topLevelArgumentBuffer);
    }
    
    for (auto& buffer : mHandle->constantBuffers)
    {
        if (buffer.IsValid())
        {
            mDevice->Dispose(buffer);
        }
    }
    
    mDevice->Dispose(mStagingHeap);
}

void CommandBuffer::BeginRenderPass(const RenderPassDescriptor& renderPassDesc, const TStringView debugName) const
{
    MTLRenderPassDescriptor* renderPass = [MTLRenderPassDescriptor renderPassDescriptor];
    
    mHandle->sampleCount = renderPassDesc.samples;
    mHandle->hasDepthAttachment = renderPassDesc.depthAttachment.texture.IsValid();
    if (mHandle->hasDepthAttachment)
    {
        mHandle->depthAttachment = renderPassDesc.depthAttachment.texture.GetDescriptor();
        
        if (Utils::IsDepthFormat(mHandle->depthAttachment.format))
        {
            MTLRenderPassDepthAttachmentDescriptor* depthAttachmentDesc = renderPass.depthAttachment;
            depthAttachmentDesc.clearDepth = renderPassDesc.depthAttachment.clearDepth;
            depthAttachmentDesc.loadAction = AttachmentLoadActionToMTLLoadAction(renderPassDesc.depthAttachment.loadAction);
            depthAttachmentDesc.storeAction = AttachmentStoreActionToMTLStoreAction(renderPassDesc.depthAttachment.storeAction);
            
            if (renderPassDesc.samples > 1)
            {
                depthAttachmentDesc.texture = renderPassDesc.depthAttachment.texture.GetMSAAHandle();
                depthAttachmentDesc.resolveTexture = renderPassDesc.depthAttachment.texture.GetHandle();
            }
            else
            {
                depthAttachmentDesc.texture = renderPassDesc.depthAttachment.texture.GetHandle();
            }
        }
        
        if (Utils::IsDepthStencilFormat(mHandle->depthAttachment.format))
        {
            MTLRenderPassStencilAttachmentDescriptor* stencilAttachmentDesc = renderPass.stencilAttachment;
            stencilAttachmentDesc.clearStencil = renderPassDesc.depthAttachment.clearStencil;
            stencilAttachmentDesc.loadAction = renderPass.depthAttachment.loadAction;
            stencilAttachmentDesc.storeAction = renderPass.depthAttachment.storeAction;
            stencilAttachmentDesc.texture = renderPass.depthAttachment.texture;
            stencilAttachmentDesc.resolveTexture = renderPass.depthAttachment.resolveTexture;
        }
    }
    
    mHandle->colorAttachments.resize(renderPassDesc.colorAttachments.size());
    for (uint32_t i = 0; i < renderPassDesc.colorAttachments.size(); i++)
    {
        const auto& colorAttachment = renderPassDesc.colorAttachments[i];
        mHandle->colorAttachments[i] = colorAttachment.texture.GetDescriptor();
        
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
        
        if (colorAttachment.texture.IsValid())
        {
            if (renderPassDesc.samples > 1)
            {
                colorAttachmentDesc.texture = colorAttachment.texture.GetMSAAHandle();
                colorAttachmentDesc.resolveTexture = colorAttachment.texture.GetHandle();
            }
            else
            {
                colorAttachmentDesc.texture = colorAttachment.texture.GetHandle();
            }
        }
        else
        {
            colorAttachmentDesc.texture = mHandle->device->AcquireNextDrawable().texture;
        }
    }
    
    mHandle->renderCommandEncoder = [mHandle->commandBuffer renderCommandEncoderWithDescriptor:renderPass];
    mHandle->renderCommandEncoder.label = TO_NSSTRING(debugName.data());
}

void CommandBuffer::EndRenderPass() const
{
    [mHandle->renderCommandEncoder endEncoding];
    mHandle->renderCommandEncoder = nil;
}

void CommandBuffer::BindGraphicsPipeline(const PipelineStateDescriptor& pipelineDesc, const Shader& vertexShader, const Shader& fragmentShader) const
{
    if (mHandle->hasDepthAttachment)
    {
        mHandle->pipeline = MetalPipelineStateManager::GetGraphicsPipeline(pipelineDesc, mHandle->colorAttachments, mHandle->depthAttachment, vertexShader, fragmentShader, mHandle->sampleCount);
        [mHandle->renderCommandEncoder setRenderPipelineState:mHandle->pipeline->handle];
        [mHandle->renderCommandEncoder setDepthStencilState:mHandle->pipeline->depthStencil];
    }
    else
    {
        mHandle->pipeline = MetalPipelineStateManager::GetGraphicsPipeline(pipelineDesc, mHandle->colorAttachments, vertexShader, fragmentShader, mHandle->sampleCount);
        [mHandle->renderCommandEncoder setRenderPipelineState:mHandle->pipeline->handle];
    }
    [mHandle->renderCommandEncoder setCullMode:CullModeToMTLCullMode(pipelineDesc.cullingMode)];
    [mHandle->renderCommandEncoder setTriangleFillMode:pipelineDesc.wireframe ? MTLTriangleFillModeLines : MTLTriangleFillModeFill];
    
    // Descriptor heap
    [mHandle->renderCommandEncoder setVertexBuffer:mHandle->device->GetCbvSrvUavHeap() offset:0 atIndex:kIRDescriptorHeapBindPoint];
    [mHandle->renderCommandEncoder setFragmentBuffer:mHandle->device->GetCbvSrvUavHeap() offset:0 atIndex:kIRDescriptorHeapBindPoint];
    
    // Top-level argument buffer
    if (mHandle->topLevelArgumentBuffer.IsValid())
    {
        mDevice->Dispose(mHandle->topLevelArgumentBuffer);
    }
    
    BufferDescriptor argumentBufferDesc;
    argumentBufferDesc.name = "CommandBuffer::TopLevelArgumentBuffer";
    argumentBufferDesc.size = MetalPipelineStateManager::GetTopLevelArgumentBufferSize();
    mHandle->topLevelArgumentBuffer = mStagingHeap.CreateBuffer(argumentBufferDesc);
    [mHandle->renderCommandEncoder setVertexBuffer:mHandle->topLevelArgumentBuffer.GetHandle() offset:0 atIndex:kIRArgumentBufferBindPoint];
    [mHandle->renderCommandEncoder setFragmentBuffer:mHandle->topLevelArgumentBuffer.GetHandle() offset:0 atIndex:kIRArgumentBufferBindPoint];
    
    // Root constants
    for (auto& buffer : mHandle->constantBuffers)
    {
        if (buffer.IsValid())
        {
            mDevice->Dispose(buffer);
        }
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

void CommandBuffer::SetConstantBuffer(const void* data, uint32_t size, uint32_t slot) const
{
    TStringStream name;
    name << "CommandBuffer::ConstantBuffer_" << slot;
    
    BufferDescriptor constantBufferDesc;
    constantBufferDesc.name = name.str();
    constantBufferDesc.size = size;
    auto buffer = mStagingHeap.CreateBuffer(constantBufferDesc);
    SetBufferData(buffer, data, size);
    
    auto argumentBufferPtr = static_cast<uint64_t*>(mHandle->topLevelArgumentBuffer.GetContents());
    argumentBufferPtr[slot] = [buffer.GetHandle() gpuAddress];
    
    [mHandle->renderCommandEncoder useResource:buffer.GetHandle() usage:MTLResourceUsageRead stages:MTLRenderStageVertex | MTLRenderStageFragment];
    mHandle->constantBuffers[slot] = buffer;
}

void CommandBuffer::SetPushConstant(const void* data, uint32_t size) const
{
    auto argumentBufferPtr = static_cast<uint64_t*>(mHandle->topLevelArgumentBuffer.GetContents());
    memcpy(argumentBufferPtr + PUSH_CONSTANT_SLOT, data, size);
}

void CommandBuffer::Draw(uint32_t vertexCount, uint32_t instanceCount) const
{
    IRRuntimeDrawPrimitives(mHandle->renderCommandEncoder, mHandle->pipeline->topology, 0, vertexCount, instanceCount, 0);
}

void CommandBuffer::DrawIndexed(const Buffer& indexBuffer, IndexType type, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex) const
{
    MTLIndexType indexType = static_cast<MTLIndexType>(type);
    IRRuntimeDrawIndexedPrimitives(mHandle->renderCommandEncoder, mHandle->pipeline->topology, indexCount, indexType, indexBuffer.GetHandle(), firstIndex * SizeOfIndexType(type), instanceCount);
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
    id<MTLTexture> dstTexture = destination.IsValid() ? destination.GetHandle() : mHandle->device->AcquireNextDrawable().texture;

    id<MTLBlitCommandEncoder> blitCommandEncoder = [mHandle->commandBuffer blitCommandEncoder];
    [blitCommandEncoder setLabel:TO_NSSTRING("CommandBuffer::Blit")];
    [blitCommandEncoder copyFromTexture:srcTexture toTexture:dstTexture];
    [blitCommandEncoder endEncoding];
}

void CommandBuffer::Begin() const
{
#ifdef GDEBUG
    MTLCommandBufferDescriptor* descriptor = [MTLCommandBufferDescriptor new];
    descriptor.errorOptions = MTLCommandBufferErrorOptionEncoderExecutionStatus;
    mHandle->commandBuffer = [mHandle->device->GetCommandPool() commandBufferWithDescriptor:descriptor];
#else
    mHandle->commandBuffer = [mHandle->device->GetCommandPool() commandBuffer];
#endif
    mCommitted = false;
}

void CommandBuffer::End() const
{
    [mHandle->commandBuffer enqueue];
}

void CommandBuffer::Commit() const
{
    [mHandle->commandBuffer commit];
    mCommitted = true;
    
    if (mHandle->topLevelArgumentBuffer.IsValid())
    {
        mDevice->Dispose(mHandle->topLevelArgumentBuffer);
    }
    mStagingHeap.Reset();
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

NativeGraphicsHandle CommandBuffer::GetActiveRenderPass() const
{
    return mHandle->renderCommandEncoder;
}

#endif
