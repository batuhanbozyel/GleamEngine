#include "gpch.h"
#include "GraphicsDevice.h"

using namespace Gleam;

Heap GraphicsDevice::CreateHeap(const HeapDescriptor& descriptor)
{
    auto it = std::find_if(mFreeHeaps.begin(), mFreeHeaps.end(), [&](const Heap& heap) -> bool
    {
        return heap.GetDescriptor().memoryType == descriptor.memoryType
			&& heap.GetDescriptor().size >= descriptor.size;
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
    return mShaderCache.emplace_back(CompileShader(entryPoint, stage));
}

GraphicsPipelineHandle GraphicsDevice::CreateGraphicsPipeline(const GraphicsPipelineStateDescriptor& pipelineDesc)
{
	GraphicsPipelineHandle handle{ std::hash<GraphicsPipelineStateDescriptor>()(pipelineDesc) };
	auto it = mGraphicsPipelineCache.find(handle);
	if (it != mGraphicsPipelineCache.end())
	{
		return handle;
	}

	auto pipeline = CompileGraphicsPipeline(pipelineDesc);
	mShaderPipelineReferences[pipelineDesc.vertexEntry].insert(handle);
	mShaderPipelineReferences[pipelineDesc.fragmentEntry].insert(handle);
	mGraphicsPipelineCache.emplace_hint(mGraphicsPipelineCache.end(), handle, pipeline);
	return handle;
}

void GraphicsDevice::ReleaseHeap(const Heap& heap)
{
    GLEAM_ASSERT(heap.IsValid());
    AddPooledObject([this, heap = heap]() mutable
    {
        heap.Reset();
        mFreeHeaps.push_back(heap);
    });
}

void GraphicsDevice::ReleaseBuffer(const Buffer& buffer)
{
	GLEAM_ASSERT(buffer.IsValid());
	AddPooledObject([this, buffer = buffer]() mutable
	{
		Dispose(buffer);
	});
}

void GraphicsDevice::ReleaseTexture(const Texture& texture)
{
    GLEAM_ASSERT(texture.IsValid());
    AddPooledObject([this, texture = texture]() mutable
    {
        mFreeTextures.push_back(texture);
    });
}

void GraphicsDevice::DestroySizeDependentResources()
{
	for (auto& texture : mFreeTextures)
	{
		Dispose(texture);
	}
	mFreeTextures.clear();
}

void GraphicsDevice::DestroyResources()
{
	DestroyPooledObjects();
	DestroySizeDependentResources();
    
    for (auto& heap : mFreeHeaps)
    {
        Dispose(heap);
    }
    mFreeHeaps.clear();
}

void GraphicsDevice::DestroyPooledObjects()
{
	for (uint32_t i = 0; i < mPooledObjects.size(); i++)
	{
		DestroyPooledObjects(i);
	}
}

void GraphicsDevice::DestroyPooledObjects(uint32_t frameIndex)
{
	DestroyFrameObjects(frameIndex);

	auto& pooledObjects = mPooledObjects[frameIndex];
	for (auto& deallocator : pooledObjects)
	{
		deallocator();
	}
	pooledObjects.clear();

	// TODO: implement a better algorithm to retrieve heaps so that always the closest fitting heap is returned
	std::sort(mFreeHeaps.begin(), mFreeHeaps.end(), [](const Heap& left, const Heap& right)
	{
		return left.GetDescriptor().size < right.GetDescriptor().size;
	});
}

const GraphicsPipeline& GraphicsDevice::GetGraphicsPipeline(GraphicsPipelineHandle handle) const
{
	auto it = mGraphicsPipelineCache.find(handle);
	if (it != mGraphicsPipelineCache.end())
	{
		return it->second;
	}

	GLEAM_ASSERT(false);
	static GraphicsPipeline invalid;
	return invalid;
}

Texture GraphicsDevice::GetRenderSurface() const
{
	return Texture({ .size = mSize,
					 .format = mFormat,
					 .usage = TextureUsage_Attachment,
					 .dimension = TextureDimension::Texture2D });
}

TextureFormat GraphicsDevice::GetFormat() const
{
	return mFormat;
}

uint32_t GraphicsDevice::GetLastFrameIndex() const
{
	return (mCurrentFrameIndex + (mMaxFramesInFlight - 1)) % mMaxFramesInFlight;
}

uint32_t GraphicsDevice::GetFrameIndex() const
{
	return mCurrentFrameIndex;
}

uint32_t GraphicsDevice::GetFramesInFlight() const
{
	return mMaxFramesInFlight;
}

const Size& GraphicsDevice::GetDrawableSize() const
{
	return mSize;
}
