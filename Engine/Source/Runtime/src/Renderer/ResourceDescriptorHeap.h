//
//  ResourceDescriptorHeap.h
//  Runtime
//
//  Created by Batuhan Bozyel on 04.03.2024.
//

#pragma once
#include "Shaders/ShaderInterop.h"
#include "Container/Array.h"

namespace Gleam {

class ResourceDescriptorHeap final
{
public:
    
    ResourceDescriptorHeap() = default;
    
	ResourceDescriptorHeap(uint32_t capacity);

	ShaderResourceIndex Allocate();

	void Release(ShaderResourceIndex index);

	uint32_t GetSize() const;

	uint32_t GetCapacity() const;

	bool Empty() const;
    
    void Reset();
    
private:
    
    uint32_t mSize = 0;

	uint32_t mCapacity = 0;
    
    TArray<ShaderResourceIndex> mDenseArray;
    
    TArray<ShaderResourceIndex> mSparseArray;
    
};

} // namespace Gleam

