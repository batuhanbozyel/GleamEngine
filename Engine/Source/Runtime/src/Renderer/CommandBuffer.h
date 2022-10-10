#pragma once
#include "Buffer.h"
#include "Shader.h"
#include "RenderTarget.h"

namespace Gleam {

struct RenderPassDescriptor;
struct PipelineStateDescriptor;

class CommandBuffer final
{
public:
    
    CommandBuffer();

    ~CommandBuffer();

    void BeginRenderPass(const RenderPassDescriptor& renderPassDesc) const;

    void EndRenderPass() const;

    void BindPipeline(const PipelineStateDescriptor& pipelineDesc, const GraphicsShader& program) const;

    void SetViewport(const Size& size) const;
    
    // Set to null for swapchain target
    void SetRenderTarget(const RefCounted<RenderTarget>& renderTarget)
    {
        mActiveRenderTarget = renderTarget;
    }

    template<typename T, BufferUsage usage, MemoryType memoryType>
    void SetVertexBuffer(const Buffer<T, usage, memoryType>& buffer, uint32_t index, uint32_t offset = 0) const
    {
        SetVertexBuffer(buffer.GetHandle(), usage, buffer.GetSize(), index, offset);
    }

    template<typename T, BufferUsage usage, MemoryType memoryType>
    void SetFragmentBuffer(const Buffer<T, usage, memoryType>& buffer, uint32_t index, uint32_t offset = 0) const
    {
		SetFragmentBuffer(buffer.GetHandle(), usage, buffer.GetSize(), index, offset);
    }

    template<typename T>
    void SetPushConstant(const T& t, ShaderStageFlagBits stage) const
    {
        SetPushConstant(&t, sizeof(T), stage);
    }

    void Draw(uint32_t vertexCount, uint32_t instanceCount = 1, uint32_t baseVertex = 0, uint32_t baseInstance = 0) const;

    template<IndexBufferTypes T, MemoryType memoryType>
    void DrawIndexed(const Buffer<T, BufferUsage::IndexBuffer, memoryType>& indexBuffer, uint32_t indexCount, uint32_t instanceCount = 1, uint32_t firstIndex = 0, uint32_t baseVertex = 0, uint32_t baseInstance = 0) const
    {
        DrawIndexed(indexBuffer.GetHandle(), IndexTraits<T>::indexType, indexCount, instanceCount, firstIndex, baseVertex, baseInstance);
    }
    
    void CopyBuffer(const IBuffer& src, const IBuffer& dst, size_t size, uint32_t srcOffset = 0, uint32_t dstOffset = 0) const;
    
    // Set to Null for swapchain target
    void Blit(const Texture& texture, const Optional<Texture>& renderTarget) const;

    void Begin() const;

    void End() const;

    void Commit() const;

    void WaitUntilCompleted() const;

private:

    void SetVertexBuffer(const NativeGraphicsHandle buffer, BufferUsage usage, uint32_t size, uint32_t index, uint32_t offset) const;

    void SetFragmentBuffer(const NativeGraphicsHandle buffer, BufferUsage usage, uint32_t size, uint32_t index, uint32_t offset) const;

    void DrawIndexed(const NativeGraphicsHandle indexBuffer, IndexType type, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, uint32_t baseVertex, uint32_t baseInstance) const;

    void SetPushConstant(const void* data, uint32_t size, ShaderStageFlagBits stage) const;

    class Impl;
    Scope<Impl> mHandle;
    
    RefCounted<RenderTarget> mActiveRenderTarget = nullptr;
    
};

} // namespace Gleam
