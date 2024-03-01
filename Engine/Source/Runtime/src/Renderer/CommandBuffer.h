#pragma once
#include "Heap.h"
#include "Buffer.h"
#include "Shader.h"
#include "Texture.h"
#include "RenderGraph/RenderGraphResource.h"

namespace Gleam {

struct RenderPassDescriptor;
struct PipelineStateDescriptor;

class GraphicsDevice;

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

    CommandBuffer(GraphicsDevice* device);

    ~CommandBuffer();

    void BeginRenderPass(const RenderPassDescriptor& renderPassDesc, const TStringView debugName = "") const;

    void EndRenderPass() const;

    void BindGraphicsPipeline(const PipelineStateDescriptor& pipelineDesc,
		const Shader& vertexShader,
		const Shader& fragmentShader) const;

    void SetViewport(const Size& size) const;

    void BindBuffer(const BufferHandle& handle,
		size_t offset,
		uint32_t index,
		ShaderStageFlagBits stage) const;

    void BindBuffer(const Buffer& buffer,
		size_t offset,
		uint32_t index,
		ShaderStageFlagBits stage,
		ResourceAccess access = ResourceAccess::Read) const;

    void BindTexture(const TextureHandle& handle,
		uint32_t index,
		ShaderStageFlagBits stage) const;

    void BindTexture(const Texture& texture,
		uint32_t index,
		ShaderStageFlagBits stage,
		ResourceAccess access = ResourceAccess::Read) const;

    template<typename T>
    void SetPushConstant(const T& t, ShaderStageFlagBits stage) const
    {
        SetPushConstant(&t, sizeof(T), stage);
    }

    void Draw(uint32_t vertexCount,
		uint32_t instanceCount = 1,
		uint32_t baseVertex = 0,
		uint32_t baseInstance = 0) const;

	void DrawIndexed(const Buffer& indexBuffer, IndexType type,
		uint32_t indexCount,
		uint32_t instanceCount = 1,
		uint32_t firstIndex = 0,
		uint32_t baseVertex = 0,
		uint32_t baseInstance = 0) const;

    void DrawIndexed(const Buffer& indexBuffer, IndexType type,
		uint32_t instanceCount = 1,
		uint32_t firstIndex = 0,
		uint32_t baseVertex = 0,
		uint32_t baseInstance = 0) const;

	void CopyBuffer(const Buffer& src, const Buffer& dst,
		size_t size,
		size_t srcOffset = 0,
		size_t dstOffset = 0) const;

    void CopyBuffer(const Buffer& src, const Buffer& dst) const;

    void SetBufferData(const Buffer& buffer, const void* data, size_t size, size_t offset = 0) const;

    void Blit(const Texture& texture, const Texture& renderTarget) const;

    void TransitionLayout(const Texture& texture, ResourceAccess access) const;

    void Begin() const;

    void End() const;

    void Commit() const;

    void WaitUntilCompleted() const;

    NativeGraphicsHandle GetHandle() const;

    NativeGraphicsHandle GetActiveRenderPass() const;

private:

    void BindBuffer(const NativeGraphicsHandle buffer,
		BufferUsage usage,
		size_t offset,
		uint32_t index,
		ShaderStageFlagBits stage,
		ResourceAccess access) const;

    void BindTexture(const NativeGraphicsHandle texture,
		uint32_t index,
		ShaderStageFlagBits stage,
		ResourceAccess access) const;

    void SetPushConstant(const void* data, uint32_t size, ShaderStageFlagBits stage) const;

	void CopyBuffer(const NativeGraphicsHandle src, const NativeGraphicsHandle dst,
		size_t size,
		size_t srcOffset = 0,
		size_t dstOffset = 0) const;

    class Impl;
    Scope<Impl> mHandle;
    
    Heap mStagingHeap;

    GraphicsDevice* mDevice;

	mutable bool mCommitted = false;
    
};

} // namespace Gleam
