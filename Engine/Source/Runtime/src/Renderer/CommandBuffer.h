#pragma once
#include "Math/Rect.h"
#include "Container/Pointer.h"

#include "Buffer.h"
#include "Shader.h"
#include "Barrier.h"
#include "Texture.h"
#include "Pipeline.h"
#include "ConstantBuffer.h"
#include "RenderPassDescriptor.h"
#include "AccelerationStructure.h"

namespace Gleam {

class GraphicsDevice;
class GPUAllocator;

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

    CommandBuffer(GraphicsDevice* device, GPUAllocator* transientAllocator);

    ~CommandBuffer();

    void BeginRenderPass(const RenderPassDescriptor& renderPassDesc, const TStringView debugName) const;

    void EndRenderPass() const;

	void BeginComputePass(const TStringView debugName) const;

	void EndComputePass() const;

	void BindComputePipeline(const ComputePipeline& pipeline) const;

    void BindGraphicsPipeline(const GraphicsPipeline& pipeline) const;

	void BindRayTracingPipeline(const RayTracingPipeline& pipeline) const;

	void BindMeshPipeline(const MeshPipeline& pipeline) const;

    void SetViewport(const Size& size) const;

	void SetScissorRect(const Rect& rect) const;
    
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

	void DispatchRays(uint32_t width, uint32_t height, uint32_t depth = 1) const;

	void DispatchRaysIndirect(const Buffer& argsBuffer, size_t offset = 0) const;

	void Dispatch(uint32_t x, uint32_t y, uint32_t z) const;

	void DispatchMesh(uint32_t x, uint32_t y, uint32_t z) const;

    void Draw(uint32_t vertexCount, uint32_t instanceCount = 1) const;

	void DrawIndexed(const Buffer& indexBuffer, IndexType type,
		uint32_t indexCount,
		uint32_t instanceCount = 1,
		uint32_t firstIndex = 0) const;

    void DrawIndexed(const Buffer& indexBuffer, IndexType type,
		uint32_t instanceCount = 1,
		uint32_t firstIndex = 0) const;

	void DrawIndirect(const Buffer& argsBuffer, size_t offset = 0) const;

	void DrawIndexedIndirect(const Buffer& indexBuffer, IndexType type,
		const Buffer& argsBuffer, size_t offset = 0) const;

	void DispatchIndirect(const Buffer& argsBuffer, size_t offset = 0) const;

	void DispatchMeshIndirect(const Buffer& argsBuffer, size_t offset = 0) const;

	void CopyBuffer(const Buffer& src, const Buffer& dst,
		size_t size,
		size_t srcOffset = 0,
		size_t dstOffset = 0) const;

    void CopyBuffer(const Buffer& src, const Buffer& dst) const;

	void ClearBuffer(const Buffer& buffer, uint8_t value = 0) const;

    void Blit(const Texture& source, const Texture& destination) const;

	void Barrier(const BarrierGroup& barrier) const;

    void Begin(const TStringView debugName) const;

    void End() const;

    void Commit() const;

    void WaitUntilCompleted() const;

    NativeGraphicsHandle GetHandle() const;

private:
    
    void SetConstantBuffer(const void* data, uint32_t size, uint32_t slot) const;

    void SetPushConstant(const void* data, uint32_t size) const;

	void CopyBuffer(const NativeGraphicsHandle src, const NativeGraphicsHandle dst,
		size_t size,
		size_t srcOffset = 0,
		size_t dstOffset = 0) const;

    struct Impl;
    Scope<Impl> mHandle;

    GraphicsDevice* mDevice;

	GPUAllocator* mTransientAllocator;

	mutable ConstantBuffer mConstantBuffer;

	mutable bool mCommitted = false;
    
};

} // namespace Gleam
