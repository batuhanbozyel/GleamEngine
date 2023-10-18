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
    
    Texture GetTexture() const
    {
        return Texture({ .size = mSize,
                         .format = mFormat,
                         .type = TextureType::RenderTexture });
    }
    
    TextureFormat GetFormat() const
    {
        return mFormat;
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
    
    void AddPooledObject(std::any object, std::function<void(std::any)> deallocator)
    {
        mPooledObjects[mCurrentFrameIndex].push_back(std::make_pair(object, deallocator));
    }
    
protected:
    
    using PooledObject = std::pair<std::any, std::function<void(std::any)>>;
    using ObjectPool = TArray<PooledObject>;
    TArray<ObjectPool> mPooledObjects;
    
    uint32_t mMaxFramesInFlight = 3;
    
    uint32_t mCurrentFrameIndex = 0;
    
    TextureFormat mFormat = TextureFormat::None;
    
    Size mSize = Size::zero;
    
};

} // namespace Gleam

