#include "gpch.h"
#include "RendererContext.h"

using namespace Gleam;

Heap RendererContext::CreateHeap(const HeapDescriptor& descriptor)
{
    for (const auto& heap : mFreeHeaps)
    {
        if (heap.GetDescriptor() == descriptor)
        {
            return heap;
        }
    }
    return Heap(descriptor);
}

Texture RendererContext::CreateTexture(const TextureDescriptor& descriptor)
{
    for (const auto& texture : mFreeTextures)
    {
        if (texture.GetDescriptor() == descriptor)
        {
            return texture;
        }
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
