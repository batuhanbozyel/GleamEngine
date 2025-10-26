#pragma once
#include "RenderSurface.h"
#include "CommandBuffer.h"
#include "RendererConfig.h"
#include "ResourceReleaseQueue.h"
#include "ResourceDescriptorHeap.h"

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

    Texture CreateTexture(const TextureDescriptor& descriptor);

    Shader CreateShader(const TString& entryPoint, ShaderStage stage);

	GraphicsPipelineHandle CreateGraphicsPipeline(const GraphicsPipelineStateDescriptor& pipelineDesc);

    void Dispose(Heap& heap);

    void Dispose(Texture& texture);

	void Dispose(Shader& shader);

	void Dispose(GraphicsPipeline& pipeline);

	const GraphicsPipeline& GetGraphicsPipeline(GraphicsPipelineHandle handle) const;
    
    MemoryRequirements QueryMemoryRequirements(const HeapDescriptor& descriptor) const;
    
protected:

	// Implemented by the backend
	virtual void Configure(const RendererConfig& config) = 0;
    
    virtual ShaderResourceIndex CreateResourceView(const Buffer& buffer) = 0;
    
    virtual ShaderResourceIndex CreateResourceView(const Texture& texture) = 0;
    
    virtual void ReleaseResourceView(ShaderResourceIndex view) = 0;

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
