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
    
    template<typename T>
    void SetConstantBuffer(const T& t, uint32_t slot) const
    {
        SetConstantBuffer(&t, sizeof(T), slot);
    }

    template<typename T>
    void SetPushConstant(const T& t) const
    {
        static_assert(sizeof(T) <= PUSH_CONSTANT_SIZE, "Push constant limit is 128 bytes.");
        SetPushConstant(&t, sizeof(T));
    }

    void Draw(uint32_t vertexCount, uint32_t instanceCount = 1) const;

	void DrawIndexed(const Buffer& indexBuffer, IndexType type,
		uint32_t indexCount,
		uint32_t instanceCount = 1,
		uint32_t firstIndex = 0) const;

    void DrawIndexed(const Buffer& indexBuffer, IndexType type,
		uint32_t instanceCount = 1,
		uint32_t firstIndex = 0) const;

	void CopyBuffer(const Buffer& src, const Buffer& dst,
		size_t size,
		size_t srcOffset = 0,
		size_t dstOffset = 0) const;

    void CopyBuffer(const Buffer& src, const Buffer& dst) const;

	template<typename T>
	void SetBufferData(const Buffer& buffer, const T& data, size_t offset = 0) const
	{
		SetBufferData(buffer, &data, sizeof(T), offset);
	}

    void SetBufferData(const Buffer& buffer, const void* data, size_t size, size_t offset = 0) const;

    void Blit(const Texture& source, const Texture& destination) const;

    void Begin() const;

    void End() const;

    void Commit() const;

    void WaitUntilCompleted() const;

    NativeGraphicsHandle GetHandle() const;

    NativeGraphicsHandle GetActiveRenderPass() const;

private:
    
    void SetConstantBuffer(const void* data, uint32_t size, uint32_t slot) const;

    void SetPushConstant(const void* data, uint32_t size) const;

	void CopyBuffer(const NativeGraphicsHandle src, const NativeGraphicsHandle dst,
		size_t size,
		size_t srcOffset = 0,
		size_t dstOffset = 0) const;

    struct Impl;
    Scope<Impl> mHandle;
    
    Heap mStagingHeap;

    GraphicsDevice* mDevice;

	mutable bool mCommitted = false;
    
};

} // namespace Gleam
