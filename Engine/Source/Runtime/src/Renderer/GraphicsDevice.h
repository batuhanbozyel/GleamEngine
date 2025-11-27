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

    Shader CreateShader(const TString& entryPoint, ShaderStage stage);

	GraphicsPipelineHandle CreateGraphicsPipeline(const GraphicsPipelineStateDescriptor& pipelineDesc);

    void Dispose(Heap& heap);

	void Dispose(GPUAllocator* allocator, Texture& texture);

	void Dispose(GPUAllocator* allocator, Buffer& buffer);

	void Dispose(Shader& shader);

	void Dispose(GraphicsPipeline& pipeline);

	const GraphicsPipeline& GetGraphicsPipeline(GraphicsPipelineHandle handle) const;
    
protected:

	// Implemented by the backend
	virtual void Configure(const RendererConfig& config) = 0;

	virtual void ResetCommandPools(uint32_t frameIdx) {};

    TArray<Shader> mShaderCache;

	HashMap<TString, HashSet<PipelineHandle>> mShaderPipelineReferences;

	HashMap<GraphicsPipelineHandle, GraphicsPipeline> mGraphicsPipelineCache;

	RenderSurface* mSurface = nullptr;

	ResourceReleaseQueue* mReleaseQueue = nullptr;

private:

	Shader CompileShader(const TString& entryPoint, ShaderStage stage);

	GraphicsPipeline CompileGraphicsPipeline(const GraphicsPipelineStateDescriptor& pipelineDesc);

};

} // namespace Gleam
