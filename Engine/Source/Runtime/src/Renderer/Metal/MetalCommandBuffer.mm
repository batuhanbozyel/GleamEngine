#include "gpch.h"

#ifdef USE_METAL_RENDERER
#include "Renderer/CommandBuffer.h"
#include "MetalPipelineStateManager.h"
#include "MetalShaderReflect.h"

#include "Core/Application.h"

#define IR_PRIVATE_IMPLEMENTATION
#include <metal_irconverter_runtime/metal_irconverter_runtime.h>

using namespace Gleam;

struct CommandBuffer::Impl
{
    id<MTLCommandBuffer> commandBuffer = nil;
    id<MTLRenderCommandEncoder> renderCommandEncoder = nil;
    id<MTLBuffer> topLevelArgumentBuffer = nil;
    const MetalPipeline* pipeline = nullptr;
    
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

void CommandBuffer::BeginRenderPass(const RenderPassDescriptor& renderPassDesc, const TStringView debugName) const
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
            
            if (renderPassDesc.samples > 1)
            {
                depthAttachmentDesc.texture = renderPassDesc.depthAttachment.texture->GetMSAAHandle();
                depthAttachmentDesc.resolveTexture = renderPassDesc.depthAttachment.texture->GetHandle();
            }
            else
            {
                depthAttachmentDesc.texture = renderPassDesc.depthAttachment.texture->GetHandle();
            }
        }
        
        if (Utils::IsStencilFormat(mHandle->depthAttachment.format))
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
        
        if (colorAttachment.texture->GetHandle() == nil)
        {
            colorAttachmentDesc.texture = MetalDevice::GetSwapchain().AcquireNextDrawable().texture;
        }
        else
        {
            if (renderPassDesc.samples > 1)
            {
                colorAttachmentDesc.texture = colorAttachment.texture->GetMSAAHandle();
                colorAttachmentDesc.resolveTexture = colorAttachment.texture->GetHandle();
            }
            else
            {
                colorAttachmentDesc.texture = colorAttachment.texture->GetHandle();
            }
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

void CommandBuffer::BindGraphicsPipeline(const PipelineStateDescriptor& pipelineDesc, const RefCounted<Shader>& vertexShader, const RefCounted<Shader>& fragmentShader) const
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
    
    auto argumentBufferSize = vertexShader->GetReflection()->argumentBufferSize + fragmentShader->GetReflection()->argumentBufferSize;
    if (argumentBufferSize > 0)
    {
        mHandle->topLevelArgumentBuffer = [MetalDevice::GetHandle() newBufferWithLength:argumentBufferSize options:MTLResourceStorageModeShared];
        [mHandle->renderCommandEncoder setVertexBuffer:mHandle->topLevelArgumentBuffer offset:0 atIndex:kIRArgumentBufferBindPoint];
        [mHandle->renderCommandEncoder setFragmentBuffer:mHandle->topLevelArgumentBuffer offset:0 atIndex:kIRArgumentBufferBindPoint];
        auto argumentBufferPtr = static_cast<uint8_t*>([mHandle->topLevelArgumentBuffer contents]);
        
        // Set vertex samplers
        for (const auto& resource : vertexShader->GetReflection()->samplers)
        {
            auto entry = As<IRDescriptorTableEntry*>(argumentBufferPtr + resource.topLevelOffset);
            IRDescriptorTableSetSampler(entry, MetalPipelineStateManager::GetSamplerState(resource.slot), 0.0f);
        }
        
        // Set fragment samplers
        for (const auto& resource : fragmentShader->GetReflection()->samplers)
        {
            auto entry = As<IRDescriptorTableEntry*>(argumentBufferPtr + resource.topLevelOffset);
            IRDescriptorTableSetSampler(entry, MetalPipelineStateManager::GetSamplerState(resource.slot), 0.0f);
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

void CommandBuffer::BindBuffer(const NativeGraphicsHandle buffer, BufferUsage usage, size_t offset, uint32_t index, ShaderStageFlagBits stage) const
{
    auto argumentBufferPtr = static_cast<uint8_t*>([mHandle->topLevelArgumentBuffer contents]);
    auto resource = [=, this]()
    {
        Shader::Reflection* reflection = nullptr;
        if (stage & ShaderStage_Vertex)
        {
            auto pipeline = static_cast<const MetalGraphicsPipeline*>(mHandle->pipeline);
            reflection = pipeline->vertexShader->GetReflection().get();
        }
        else if (stage & ShaderStage_Fragment)
        {
            auto pipeline = static_cast<const MetalGraphicsPipeline*>(mHandle->pipeline);
            reflection = pipeline->fragmentShader->GetReflection().get();
        }
        else
        {
            GLEAM_ASSERT(false, "Metal: Shader stage not implemented yet.")
        }
        
        switch (usage)
        {
            case BufferUsage::UniformBuffer: return Shader::Reflection::GetResourceFromTypeArray(reflection->CBVs, index);
            case BufferUsage::VertexBuffer:
            case BufferUsage::StorageBuffer:
            {
                return Shader::Reflection::GetResourceFromTypeArray(reflection->SRVs, index); // TODO: Add support for UAVs
            }
            default: GLEAM_ASSERT(false, "Metal: Trying to bind buffer with invalid usage.") return IRResourceLocation();
        }
    }();
    
    [mHandle->renderCommandEncoder useResource:buffer usage:MTLResourceUsageRead stages:ShaderStagesToMTLRenderStages(stage)];
    auto entry = As<IRDescriptorTableEntry*>(argumentBufferPtr + resource.topLevelOffset);
    IRDescriptorTableSetBuffer(entry, [buffer gpuAddress] + offset, 0);
}

void CommandBuffer::BindTexture(const NativeGraphicsHandle texture, uint32_t index, ShaderStageFlagBits stage) const
{
    auto argumentBufferPtr = static_cast<uint8_t*>([mHandle->topLevelArgumentBuffer contents]);
    auto resource = [=, this]()
    {
        Shader::Reflection* reflection = nullptr;
        if (stage & ShaderStage_Vertex)
        {
            auto pipeline = static_cast<const MetalGraphicsPipeline*>(mHandle->pipeline);
            reflection = pipeline->vertexShader->GetReflection().get();
        }
        else if (stage & ShaderStage_Fragment)
        {
            auto pipeline = static_cast<const MetalGraphicsPipeline*>(mHandle->pipeline);
            reflection = pipeline->fragmentShader->GetReflection().get();
        }
        else
        {
            GLEAM_ASSERT(false, "Metal: Shader stage not implemented yet.")
        }
        return Shader::Reflection::GetResourceFromTypeArray(reflection->SRVs, index); // TODO: Add support for UAVs
    }();
    
    [mHandle->renderCommandEncoder useResource:texture usage:MTLResourceUsageRead stages:ShaderStagesToMTLRenderStages(stage)];
    auto entry = As<IRDescriptorTableEntry*>(argumentBufferPtr + resource.topLevelOffset);
    IRDescriptorTableSetTexture(entry, texture, 0.0f, 0);
}

void CommandBuffer::SetPushConstant(const void* data, uint32_t size, ShaderStageFlagBits stage) const
{
//    if (stage & ShaderStage_Vertex)
//        [mHandle->renderCommandEncoder setVertexBytes:data length:size atIndex:RendererBindingTable::PushConstantBlock];
//    
//    if (stage & ShaderStage_Fragment)
//        [mHandle->renderCommandEncoder setFragmentBytes:data length:size atIndex:RendererBindingTable::PushConstantBlock];
//    
//    if (stage & ShaderStage_Compute)
//        [mHandle->renderCommandEncoder setTileBytes:data length:size atIndex:RendererBindingTable::PushConstantBlock];
}

void CommandBuffer::Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t baseVertex, uint32_t baseInstance) const
{
    IRRuntimeDrawPrimitives(mHandle->renderCommandEncoder, mHandle->pipeline->topology, baseVertex, vertexCount, instanceCount, baseInstance);
}

void CommandBuffer::DrawIndexed(const NativeGraphicsHandle indexBuffer, IndexType type, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, uint32_t baseVertex, uint32_t baseInstance) const
{
    MTLIndexType indexType = static_cast<MTLIndexType>(type);
    IRRuntimeDrawIndexedPrimitives(mHandle->renderCommandEncoder, mHandle->pipeline->topology, indexCount, indexType, indexBuffer, firstIndex * SizeOfIndexType(type), instanceCount, baseVertex, baseInstance);
}

void CommandBuffer::CopyBuffer(const NativeGraphicsHandle src, const NativeGraphicsHandle dst, size_t size, uint32_t srcOffset, uint32_t dstOffset) const
{
    id<MTLBlitCommandEncoder> blitCommandEncoder = [mHandle->commandBuffer blitCommandEncoder];
    [blitCommandEncoder copyFromBuffer:src sourceOffset:srcOffset toBuffer:dst destinationOffset:dstOffset size:size];
    [blitCommandEncoder endEncoding];
}

void CommandBuffer::Blit(const RenderTexture& texture, const RenderTexture& target) const
{
    id<MTLTexture> srcTexture = texture.GetHandle();
    id<MTLTexture> dstTexture = target.IsValid() ? target.GetHandle() : MetalDevice::GetSwapchain().AcquireNextDrawable().texture;

    id<MTLBlitCommandEncoder> blitCommandEncoder = [mHandle->commandBuffer blitCommandEncoder];
    [blitCommandEncoder copyFromTexture:srcTexture toTexture:dstTexture];
    [blitCommandEncoder endEncoding];
}

void CommandBuffer::Begin() const
{
#ifdef GDEBUG
    MTLCommandBufferDescriptor* descriptor = [MTLCommandBufferDescriptor new];
    descriptor.errorOptions = MTLCommandBufferErrorOptionEncoderExecutionStatus;
    mHandle->commandBuffer = [MetalDevice::GetCommandPool() commandBufferWithDescriptor:descriptor];
#else
    mHandle->commandBuffer = [MetalDevice::GetCommandPool() commandBuffer];
#endif
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

NativeGraphicsHandle CommandBuffer::GetHandle() const
{
    return mHandle->commandBuffer;
}

NativeGraphicsHandle CommandBuffer::GetActiveRenderPass() const
{
    return mHandle->renderCommandEncoder;
}

#endif
