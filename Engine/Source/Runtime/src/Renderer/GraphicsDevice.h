#pragma once
#include "Heap.h"
#include "Shader.h"
#include "Texture.h"
#include "RendererConfig.h"

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

	MemoryRequirements QueryMemoryRequirements(const HeapDescriptor& descriptor) const;

    void ReleaseHeap(const Heap& heap);

    void ReleaseTexture(const Texture& texture);

    void Dispose(Heap& heap) const;

    void Dispose(Buffer& buffer) const;

    void Dispose(Texture& texture) const;

	Texture GetRenderSurface() const;

	TextureFormat GetFormat() const;

	uint32_t GetLastFrameIndex() const;

	uint32_t GetFrameIndex() const;

	uint32_t GetFramesInFlight() const;

	const Size& GetDrawableSize() const;

	using ObjectDeallocator = std::function<void()>;
	void AddPooledObject(ObjectDeallocator&& deallocator)
	{
		mPooledObjects[mCurrentFrameIndex].push_back(deallocator);
	}
    
protected:

	// Implemented by the backend
	virtual void Configure(const RendererConfig& config) = 0;

	virtual void DestroyFrameObjects(uint32_t frameIndex) {}

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

    Heap AllocateHeap(const HeapDescriptor& descriptor) const;
    
    Texture AllocateTexture(const TextureDescriptor& descriptor) const;
    
    Shader GenerateShader(const TString& entryPoint, ShaderStage stage) const;

};

} // namespace Gleam
