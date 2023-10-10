#pragma once
#include "Shader.h"
#include "Buffer.h"
#include "RendererConfig.h"

namespace Gleam {

class RenderSystem;
struct Version;

class RendererContext final
{
	friend class RenderSystem;

public:
    
    Heap CreateHeap(const HeapDescriptor& descriptor);
    
    Texture CreateTexture(const TextureDescriptor& descriptor);
    
	const RefCounted<Shader>& CreateShader(const TString& entryPoint, ShaderStage stage);
    
    void ReleaseHeap(const Heap& heap);
    
    void ReleaseTexture(const Texture& texture);
    
    void Configure(const RendererConfig& config) const;
    
    void Clear();
    
    const Size& GetDrawableSize() const;

private:

	void WaitDeviceIdle() const;
    
    void ConfigureBackend();
    
    void DestroyBackend();
    
    Deque<Heap> mFreeHeaps;
    
    Deque<Texture> mFreeTextures;
    
	TArray<RefCounted<Shader>> mShaderCache;

};

} // namespace Gleam
