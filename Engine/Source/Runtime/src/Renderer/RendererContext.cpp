#include "gpch.h"
#include "RendererContext.h"

using namespace Gleam;

RendererContext::RendererContext()
{
    EventDispatcher<RendererPresentEvent>::Subscribe([this](RendererPresentEvent e)
    {
        auto& releasedTextures = mDeferredReleasedTextures[e.GetFrameIndex()];
        for (const auto& texture : releasedTextures)
        {
            GLEAM_ASSERT(texture.IsValid());
            mFreeTextures.push_back(texture);
        }
        releasedTextures.clear();
        
        auto& releasedHeaps = mDeferredReleasedHeaps[e.GetFrameIndex()];
        for (const auto& heap : releasedHeaps)
        {
            GLEAM_ASSERT(heap.IsValid());
            mFreeHeaps.push_back(heap);
        }
        releasedHeaps.clear();
    });
}

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
        GLEAM_ASSERT(heap.IsValid());
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
        GLEAM_ASSERT(texture.IsValid());
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
    GLEAM_ASSERT(heap.IsValid());
    mDeferredReleasedHeaps[GetFrameIndex()].push_back(heap);
}

void RendererContext::ReleaseTexture(const Texture& texture)
{
    GLEAM_ASSERT(texture.IsValid());
    mDeferredReleasedTextures[GetFrameIndex()].push_back(texture);
}

void RendererContext::Clear()
{
    for (auto& releasedTextures : mDeferredReleasedTextures)
    {
        for (auto& texture : releasedTextures)
        {
            texture.Dispose();
        }
        releasedTextures.clear();
    }
    
    for (auto& releasedHeaps : mDeferredReleasedHeaps)
    {
        for (auto& heap : releasedHeaps)
        {
            heap.Dispose();
        }
        releasedHeaps.clear();
    }
    
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
