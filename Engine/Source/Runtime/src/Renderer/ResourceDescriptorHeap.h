//
//  ResourceDescriptorHeap.h
//  Runtime
//
//  Created by Batuhan Bozyel on 04.03.2024.
//

#pragma once

namespace Gleam {

class ResourceDescriptorHeap final
{
public:
    
    ResourceDescriptorHeap() = default;
    
	ResourceDescriptorHeap(uint32_t capacity);

	ShaderResourceIndex Allocate();

	void Release(ShaderResourceIndex index);
    
    void Reset();
    
private:
    
    uint32_t mSize = 0;

	uint32_t mCapacity;
    
    TArray<ShaderResourceIndex> mDenseArray;
    
    TArray<ShaderResourceIndex> mSparseArray;
    
};

} // namespace Gleam

