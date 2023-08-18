#pragma once
#include "Buffer.h"
#include "Shader.h"
#include "Texture.h"

namespace Gleam {

class RenderTexture;
struct RenderPassDescriptor;
struct PipelineStateDescriptor;

enum class IndexType
{
    UINT16,
    UINT32
};

static constexpr size_t SizeOfIndexType(IndexType indexType)
{
    switch (indexType)
    {
        case IndexType::UINT16: return sizeof(uint16_t);
        case IndexType::UINT32: return sizeof(uint32_t);
        default: return 0;
    }
}

class CommandBuffer final
{
public:
    
    CommandBuffer();

    ~CommandBuffer();
    
    void BeginRenderPass(const RenderPassDescriptor& renderPassDesc, const TStringView debugName = "") const;

    void EndRenderPass() const;

	void BindGraphicsPipeline(const PipelineStateDescriptor& pipelineDesc, const RefCounted<Shader>& vertexShader, const RefCounted<Shader>& fragmentShader) const;

    void SetViewport(const Size& size) const;
    
    void BindBuffer(const Buffer& buffer, size_t offset, uint32_t index, ShaderStageFlagBits stage) const
    {
        BindBuffer(buffer.GetHandle(), buffer.GetDescriptor().usage, offset, index, stage);
    }
    
    void BindTexture(const Texture& texture, uint32_t index, ShaderStageFlagBits stage) const
    {
        BindTexture(texture.GetView(), index, stage);
    }

    template<typename T>
    void SetPushConstant(const T& t, ShaderStageFlagBits stage) const
    {
        SetPushConstant(&t, sizeof(T), stage);
    }

    void Draw(uint32_t vertexCount, uint32_t instanceCount = 1, uint32_t baseVertex = 0, uint32_t baseInstance = 0) const;
    
    void DrawIndexed(const NativeGraphicsHandle indexBuffer, IndexType type, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, uint32_t baseVertex, uint32_t baseInstance) const;
    
    void DrawIndexed(const Buffer& indexBuffer, IndexType type, uint32_t instanceCount = 1, uint32_t firstIndex = 0, uint32_t baseVertex = 0, uint32_t baseInstance = 0) const
    {
        DrawIndexed(indexBuffer.GetHandle(), type, static_cast<uint32_t>(indexBuffer.GetDescriptor().size / SizeOfIndexType(type)), instanceCount, firstIndex, baseVertex, baseInstance);
    }
    
    void CopyBuffer(const Buffer& src, const Buffer& dst) const
    {
        auto minSize = Math::Min(src.GetDescriptor().size, dst.GetDescriptor().size);
        CopyBuffer(src.GetHandle(), dst.GetHandle(), minSize);
    }
    
    void CopyBuffer(const NativeGraphicsHandle src, const NativeGraphicsHandle dst, size_t size, uint32_t srcOffset = 0, uint32_t dstOffset = 0) const;
    
    void Blit(const RenderTexture& texture, const RenderTexture& renderTarget) const;

    void Begin() const;

    void End() const;

    void Commit() const;

	void Present() const;

    void WaitUntilCompleted() const;
    
    NativeGraphicsHandle GetHandle() const;
    
    NativeGraphicsHandle GetActiveRenderPass() const;

private:

    void BindBuffer(const NativeGraphicsHandle buffer, BufferUsage usage, size_t offset, uint32_t index, ShaderStageFlagBits stage) const;
    
    void BindTexture(const NativeGraphicsHandle texture, uint32_t index, ShaderStageFlagBits stage) const;
    
    void SetPushConstant(const void* data, uint32_t size, ShaderStageFlagBits stage) const;

    class Impl;
    Scope<Impl> mHandle;
    
};

} // namespace Gleam
