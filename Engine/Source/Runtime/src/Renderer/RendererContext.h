#pragma once
#include "Heap.h"
#include "Shader.h"
#include "RendererConfig.h"

namespace Gleam {

class RenderSystem;
struct Version;

class RendererContext final
{
	friend class RenderSystem;

public:
    
    RendererContext();
    
    Heap CreateHeap(const HeapDescriptor& descriptor);
    
    Texture CreateTexture(const TextureDescriptor& descriptor);
    
	const RefCounted<Shader>& CreateShader(const TString& entryPoint, ShaderStage stage);
    
    void ReleaseHeap(const Heap& heap);
    
    void ReleaseTexture(const Texture& texture);
    
    void Configure(const RendererConfig& config);
    
    void WaitDeviceIdle() const;
    
    void Clear();
    
    uint32_t GetFrameIndex() const;
    
    uint32_t GetFramesInFlight() const;
    
    const Size& GetDrawableSize() const;

private:
    
    void AddPooledObject(std::any object, std::function<void(std::any)> deallocator);
    
    void ConfigureBackend();
    
    void DestroyBackend();
    
    Deque<Heap> mFreeHeaps;
    
    Deque<Texture> mFreeTextures;
    
	TArray<RefCounted<Shader>> mShaderCache;

};

} // namespace Gleam
