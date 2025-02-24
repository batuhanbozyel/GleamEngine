#pragma once
#include "CommandBuffer.h"
#include "RendererConfig.h"
#include "ResourceDescriptorHeap.h"

namespace Gleam {

class RenderSystem;
struct Version;

class GraphicsDevice : public GraphicsObject
{
    friend class RenderSystem;

public:

    GLEAM_NONCOPYABLE(GraphicsDevice);

    GraphicsDevice() = default;

    virtual ~GraphicsDevice() = default;

	void DestroyResources();

	void DestroySizeDependentResources();

	void DestroyPooledObjects();

	void DestroyPooledObjects(uint32_t frameIndex);

    Heap CreateHeap(const HeapDescriptor& descriptor);

    Texture CreateTexture(const TextureDescriptor& descriptor);

    Shader CreateShader(const TString& entryPoint, ShaderStage stage);

	GraphicsPipelineHandle CreateGraphicsPipeline(const GraphicsPipelineStateDescriptor& pipelineDesc);

    void ReleaseHeap(const Heap& heap);

	void ReleaseBuffer(const Buffer& buffer);

    void ReleaseTexture(const Texture& texture);

    void Dispose(Heap& heap);

    void Dispose(Buffer& buffer);

    void Dispose(Texture& texture);

	void Dispose(Shader& shader);

	void Dispose(GraphicsPipeline& pipeline);

	const GraphicsPipeline& GetGraphicsPipeline(GraphicsPipelineHandle handle) const;
    
    MemoryRequirements QueryMemoryRequirements(const HeapDescriptor& descriptor) const;

	using ObjectDeallocator = std::function<void()>;
	void AddPooledObject(ObjectDeallocator&& deallocator)
	{
		mPooledObjects[mCurrentFrameIndex].push_back(deallocator);
	}
    
protected:

	// Implemented by the backend
	virtual void Configure(const RendererConfig& config) = 0;

	virtual void DestroyFrameObjects(uint32_t frameIndex) {}
    
    virtual ShaderResourceIndex CreateResourceView(const Buffer& buffer) = 0;
    
    virtual ShaderResourceIndex CreateResourceView(const Texture& texture) = 0;

	virtual ShaderResourceIndex CreateRenderTargetView(const NativeGraphicsHandle texture) = 0;
    
    virtual void ReleaseResourceView(ShaderResourceIndex view) = 0;

	using ObjectPool = TArray<ObjectDeallocator>;
	TArray<ObjectPool> mPooledObjects;

    Deque<Heap> mFreeHeaps;

    Deque<Texture> mFreeTextures;

    TArray<Shader> mShaderCache;

	HashMap<TString, HashSet<PipelineHandle>> mShaderPipelineReferences;

	HashMap<GraphicsPipelineHandle, GraphicsPipeline> mGraphicsPipelineCache;

private:

    Heap AllocateHeap(const HeapDescriptor& descriptor);
    
    Texture AllocateTexture(const TextureDescriptor& descriptor);

	Shader CompileShader(const TString& entryPoint, ShaderStage stage);

	GraphicsPipeline CompileGraphicsPipeline(const GraphicsPipelineStateDescriptor& pipelineDesc);

};

} // namespace Gleam
