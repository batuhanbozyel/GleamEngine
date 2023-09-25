#include "gpch.h"
#include "RendererContext.h"

using namespace Gleam;

Heap RendererContext::CreateHeap(const HeapDescriptor& descriptor)
{
    auto it = std::find_if(mFreeHeaps.begin(), mFreeHeaps.end(), [&](const Heap& heap) -> bool
    {
        return heap.GetDescriptor() == descriptor;
    });
    
    if (it != mFreeHeaps.end())
    {
        auto heap = *it;
        mFreeHeaps.erase(it);
        return heap;
    }
    return Heap(descriptor);
}

Texture RendererContext::CreateTexture(const TextureDescriptor& descriptor)
{
    auto it = std::find_if(mFreeTextures.begin(), mFreeTextures.end(), [&](const Texture& texture) -> bool
    {
        return texture.GetDescriptor() == descriptor;
    });
    
    if (it != mFreeTextures.end())
    {
        auto texture = *it;
        mFreeTextures.erase(it);
        return texture;
    }
    return Texture(descriptor);
}

const RefCounted<Shader>& RendererContext::CreateShader(const TString& entryPoint, ShaderStage stage)
{
    for (const auto& shader : mShaderCache)
    {
        if (shader->GetEntryPoint() == entryPoint)
        {
            return shader;
        }
    }
    
    return mShaderCache.emplace_back(CreateRef<Shader>(entryPoint, stage));
}

void RendererContext::ReleaseHeap(const Heap& heap)
{
    mFreeHeaps.push_back(heap);
}

void RendererContext::ReleaseTexture(const Texture& texture)
{
    mFreeTextures.push_back(texture);
}

void RendererContext::Clear()
{
    for (auto& texture : mFreeTextures)
    {
        texture.Dispose();
    }
    mFreeTextures.clear();
    
    for (auto& heap : mFreeHeaps)
    {
        heap.Dispose();
    }
    mFreeHeaps.clear();
    
    mShaderCache.clear();
}
