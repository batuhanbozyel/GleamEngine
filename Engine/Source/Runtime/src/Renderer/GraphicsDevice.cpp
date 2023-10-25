#include "gpch.h"
#include "GraphicsDevice.h"

using namespace Gleam;

Heap GraphicsDevice::CreateHeap(const HeapDescriptor& descriptor)
{
    auto it = std::find_if(mFreeHeaps.begin(), mFreeHeaps.end(), [&](const Heap& heap) -> bool
    {
        return heap.GetDescriptor() == descriptor;
    });
    
    if (it != mFreeHeaps.end())
    {
        auto heap = *it;
        GLEAM_ASSERT(heap.IsValid());
        mFreeHeaps.erase(it);
        return heap;
    }
    return AllocateHeap(descriptor);
}

Texture GraphicsDevice::CreateTexture(const TextureDescriptor& descriptor)
{
    auto it = std::find_if(mFreeTextures.begin(), mFreeTextures.end(), [&](const Texture& texture) -> bool
    {
        return texture.GetDescriptor() == descriptor;
    });
    
    if (it != mFreeTextures.end())
    {
        auto texture = *it;
        GLEAM_ASSERT(texture.IsValid());
        mFreeTextures.erase(it);
        return texture;
    }
    return AllocateTexture(descriptor);
}

Shader GraphicsDevice::CreateShader(const TString& entryPoint, ShaderStage stage)
{
    for (const auto& shader : mShaderCache)
    {
        if (shader.GetEntryPoint() == entryPoint)
        {
            return shader;
        }
    }
    return mShaderCache.emplace_back(GenerateShader(entryPoint, stage));
}

void GraphicsDevice::ReleaseHeap(const Heap& heap)
{
    GLEAM_ASSERT(heap.IsValid());
    mSwapchain->AddPooledObject(std::make_any<Heap>(heap), [this](std::any obj)
    {
        auto heap = std::any_cast<Heap>(obj);
        heap.Reset();
        mFreeHeaps.push_back(heap);
    });
}

void GraphicsDevice::ReleaseTexture(const Texture& texture)
{
    GLEAM_ASSERT(texture.IsValid());
    mSwapchain->AddPooledObject(std::make_any<Texture>(texture), [this](std::any obj)
    {
        mFreeTextures.push_back(std::any_cast<Texture>(obj));
    });
}

void GraphicsDevice::Clear()
{
    for (auto& texture : mFreeTextures)
    {
        Dispose(texture);
    }
    mFreeTextures.clear();
    
    for (auto& heap : mFreeHeaps)
    {
        Dispose(heap);
    }
    mFreeHeaps.clear();
}

Swapchain* GraphicsDevice::GetSwapchain()
{
    return mSwapchain.get();
}

const Swapchain* GraphicsDevice::GetSwapchain() const
{
    return mSwapchain.get();
}

NativeGraphicsHandle GraphicsDevice::GetHandle() const
{
    return mHandle;
}
