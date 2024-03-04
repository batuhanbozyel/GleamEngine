//
//  ResourceDescriptorHeap.cpp
//  Runtime
//
//  Created by Batuhan Bozyel on 04.03.2024.
//

#include "gpch.h"
#include "ResourceDescriptorHeap.h"

using namespace Gleam;

ResourceDescriptorHeap::ResourceDescriptorHeap(uint32_t capacity)
	: mCapacity(capacity), mDenseArray(capacity), mSparseArray(capacity)
{
    Reset();
}

ShaderResourceIndex ResourceDescriptorHeap::Allocate()
{
    if (mSize == mCapacity)
    {
        return InvalidResourceIndex;
    }
    
    uint32_t index = mSize++;
    ShaderResourceIndex handle = mDenseArray[index];
    mSparseArray[handle.data] = ShaderResourceIndex(index);
    return handle;
}

void ResourceDescriptorHeap::Release(ShaderResourceIndex index)
{
    GLEAM_ASSERT(mCapacity > 0);
    GLEAM_ASSERT(mSize > 0);
    GLEAM_ASSERT(index != InvalidResourceIndex);
    
    uint32_t lastIndex = --mSize;
    ShaderResourceIndex handle = mSparseArray[index.data];
    GLEAM_ASSERT(handle != InvalidResourceIndex, "Trying to release already released index.");
    mSparseArray[index.data] = InvalidResourceIndex;
    
    ShaderResourceIndex temp = mDenseArray[lastIndex];
    mDenseArray[lastIndex] = index;
    mDenseArray[handle.data] = temp;
    mSparseArray[temp.data] = handle;
}

void ResourceDescriptorHeap::Reset()
{
    for (uint32_t i = 0; i < mCapacity; i++)
    {
        mDenseArray[i] = ShaderResourceIndex(i);
        mSparseArray[i] = InvalidResourceIndex;
    }
    mSize = 0;
}
