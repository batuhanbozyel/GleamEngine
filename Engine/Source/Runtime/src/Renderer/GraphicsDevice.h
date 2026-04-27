#pragma once
#include "RenderSurface.h"
#include "CommandBuffer.h"
#include "RendererConfig.h"
#include "ResourceReleaseQueue.h"
#include "ResourceDescriptorHeap.h"
#include "Allocator/GPUAllocator.h"

namespace Gleam {

class RenderSystem;
struct Version;

class GraphicsDevice : public GraphicsObject
{
    friend class RenderSystem;
public:

    GLEAM_NONCOPYABLE(GraphicsDevice);

    GraphicsDevice(RenderSurface* surface, ResourceReleaseQueue* releaseQueue);

    virtual ~GraphicsDevice() = default;

    Heap CreateHeap(const HeapDescriptor& descriptor);

	Texture CreateTexture(GPUAllocator* allocator, const TextureDescriptor& descriptor);

	Buffer CreateBuffer(GPUAllocator* allocator, const BufferDescriptor& descriptor);

	BottomLevelAccelerationStructure CreateBLAS(const BLASDescriptor& descriptor);

	TopLevelAccelerationStructure CreateTLAS(const TLASDescriptor& descriptor);

    Shader CreateShader(const TString& entryPoint, ShaderStage stage);

	ComputePipelineHandle CreateComputePipeline(const ComputePipelineStateDescriptor& pipelineDesc);

	GraphicsPipelineHandle CreateGraphicsPipeline(const GraphicsPipelineStateDescriptor& pipelineDesc);

	RayTracingPipelineHandle CreateRayTracingPipeline(const RayTracingPipelineStateDescriptor& pipelineDesc);

    void Dispose(Heap& heap);

	void Dispose(GPUAllocator* allocator, Texture& texture, BarrierStage stage);

	void Dispose(GPUAllocator* allocator, Buffer& buffer, BarrierStage stage);

	void Dispose(BottomLevelAccelerationStructure& blas);

	void Dispose(TopLevelAccelerationStructure& tlas);

	void Dispose(Shader& shader);

	void Dispose(ComputePipeline& pipeline);

	void Dispose(GraphicsPipeline& pipeline);

	void Dispose(RayTracingPipeline& pipeline);

	void Dispose(ResourceReleaseQueue::ObjectDeallocator&& deallocator);

	const ComputePipeline& GetComputePipeline(ComputePipelineHandle handle) const;

	const GraphicsPipeline& GetGraphicsPipeline(GraphicsPipelineHandle handle) const;

	const RayTracingPipeline& GetRayTracingPipeline(RayTracingPipelineHandle handle) const;
    
protected:

	// Implemented by the backend
	virtual void Configure(const RendererConfig& config) = 0;

    TArray<Shader> mShaderCache;

	HashMap<TString, HashSet<PipelineHandle>> mShaderPipelineReferences;

	HashMap<ComputePipelineHandle, ComputePipeline> mComputePipelineCache;

	HashMap<GraphicsPipelineHandle, GraphicsPipeline> mGraphicsPipelineCache;

	HashMap<RayTracingPipelineHandle, RayTracingPipeline> mRayTracingPipelineCache;

	RenderSurface* mSurface = nullptr;

	ResourceReleaseQueue* mReleaseQueue = nullptr;

private:

	Shader CompileShader(const TString& entryPoint, ShaderStage stage);

	ComputePipeline CompileComputePipeline(const ComputePipelineStateDescriptor& pipelineDesc);

	GraphicsPipeline CompileGraphicsPipeline(const GraphicsPipelineStateDescriptor& pipelineDesc);

	RayTracingPipeline CompileRayTracingPipeline(const RayTracingPipelineStateDescriptor& pipelineDesc);

};

} // namespace Gleam
