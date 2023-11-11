//
//  Swapchain.h
//  Runtime
//
//  Created by Batuhan Bozyel on 17.10.2023.
//

#pragma once

namespace Gleam {

class Swapchain
{
public:

    GLEAM_NONCOPYABLE(Swapchain);

    Swapchain() = default;

    virtual ~Swapchain() = default;

    Texture GetTexture() const
    {
        return Texture({ .size = mSize,
                         .format = mFormat,
                         .type = TextureType::RenderTexture });
	}

	void FlushAll()
	{
		for (uint32_t i = 0; i < mPooledObjects.size(); i++)
		{
			Flush(i);
		}
	}
    
    virtual void Flush(uint32_t frameIndex)
    {
        auto& pooledObjects = mPooledObjects[frameIndex];
        for (auto& deallocator : pooledObjects)
        {
			deallocator();
        }
        pooledObjects.clear();
    }

    TextureFormat GetFormat() const
    {
        return mFormat;
    }
    
    uint32_t GetLastFrameIndex() const
    {
        return (mCurrentFrameIndex + (mMaxFramesInFlight - 1)) % mMaxFramesInFlight;
    }

    uint32_t GetFrameIndex() const
    {
        return mCurrentFrameIndex;
    }

    uint32_t GetFramesInFlight() const
    {
        return mMaxFramesInFlight;
    }

    const Size& GetDrawableSize() const
    {
        return mSize;
    }

	using ObjectDeallocator = std::function<void()>;
    void AddPooledObject(ObjectDeallocator&& deallocator)
    {
        mPooledObjects[mCurrentFrameIndex].push_back(deallocator);
    }
    
protected:
    
    using ObjectPool = TArray<ObjectDeallocator>;
    TArray<ObjectPool> mPooledObjects;

    uint32_t mMaxFramesInFlight = 3;

    uint32_t mCurrentFrameIndex = 0;

    TextureFormat mFormat = TextureFormat::None;

    Size mSize = Size::zero;
    
};

} // namespace Gleam

