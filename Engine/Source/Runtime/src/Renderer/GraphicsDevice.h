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

    static Scope<GraphicsDevice> Create();

    GraphicsDevice() = default;

    virtual ~GraphicsDevice() = default;

	void DestroyResources();

	void DestroySizeDependentResources();

	void DestroyPooledObjects();

	void DestroyPooledObjects(uint32_t frameIndex);

    Heap CreateHeap(const HeapDescriptor& descriptor);

    Texture CreateTexture(const TextureDescriptor& descriptor);

    Shader CreateShader(const TString& entryPoint, ShaderStage stage);

    void ReleaseHeap(const Heap& heap);

    void ReleaseTexture(const Texture& texture);

    void Dispose(Heap& heap);

    void Dispose(Buffer& buffer);

    void Dispose(Texture& texture);

	Texture GetRenderSurface() const;

	TextureFormat GetFormat() const;

	uint32_t GetLastFrameIndex() const;

	uint32_t GetFrameIndex() const;

	uint32_t GetFramesInFlight() const;

	const Size& GetDrawableSize() const;
    
    MemoryRequirements QueryMemoryRequirements(const HeapDescriptor& descriptor) const;

	using ObjectDeallocator = std::function<void()>;
	void AddPooledObject(ObjectDeallocator&& deallocator)
	{
		mPooledObjects[mCurrentFrameIndex].push_back(deallocator);
	}
    
protected:

	// Implemented by the backend
	virtual void Present(const CommandBuffer* cmd) = 0;

	virtual void Configure(const RendererConfig& config) = 0;

	virtual void DestroyFrameObjects(uint32_t frameIndex) {}
    
    virtual ShaderResourceIndex CreateResourceView(const Buffer& buffer) = 0;
    
    virtual ShaderResourceIndex CreateResourceView(const Texture& texture) = 0;
    
    virtual void ReleaseResourceView(ShaderResourceIndex view) = 0;

	using ObjectPool = TArray<ObjectDeallocator>;
	TArray<ObjectPool> mPooledObjects;

    Deque<Heap> mFreeHeaps;

    Deque<Texture> mFreeTextures;

    TArray<Shader> mShaderCache;

	uint32_t mMaxFramesInFlight = 3;

	uint32_t mCurrentFrameIndex = 0;

	TextureFormat mFormat = TextureFormat::None;

	Size mSize = Size::zero;

private:

    Heap AllocateHeap(const HeapDescriptor& descriptor);
    
    Texture AllocateTexture(const TextureDescriptor& descriptor);
    
    Shader GenerateShader(const TString& entryPoint, ShaderStage stage) const;

};

} // namespace Gleam
