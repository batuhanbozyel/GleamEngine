#pragma once
#include "Heap.h"
#include "Shader.h"
#include "Texture.h"
#include "Swapchain.h"
#include "RendererConfig.h"

namespace Gleam {

class RenderSystem;
struct Version;

class GraphicsDevice
{
    friend class RenderSystem;

public:

    GLEAM_NONCOPYABLE(GraphicsDevice);

    static Scope<GraphicsDevice> Create();

    GraphicsDevice() = default;

    virtual ~GraphicsDevice() = default;

    Heap CreateHeap(const HeapDescriptor& descriptor);

    Texture CreateTexture(const TextureDescriptor& descriptor);

    Shader CreateShader(const TString& entryPoint, ShaderStage stage);

    void ReleaseHeap(const Heap& heap);

    void ReleaseTexture(const Texture& texture);

    void Dispose(Heap& heap) const;

    void Dispose(Buffer& buffer) const;

    void Dispose(Texture& texture) const;

    void WaitDeviceIdle() const;

    Swapchain* GetSwapchain();

    const Swapchain* GetSwapchain() const;

    NativeGraphicsHandle GetHandle() const;
    
protected:

	void Clear();
	
    NativeGraphicsHandle mHandle;

    Scope<Swapchain> mSwapchain;

    Deque<Heap> mFreeHeaps;

    Deque<Texture> mFreeTextures;

    TArray<Shader> mShaderCache;

private:

	void Configure(const RendererConfig& config);

    Heap AllocateHeap(const HeapDescriptor& descriptor) const;
    
    Texture AllocateTexture(const TextureDescriptor& descriptor) const;
    
    Shader GenerateShader(const TString& entryPoint, ShaderStage stage) const;

};

} // namespace Gleam
