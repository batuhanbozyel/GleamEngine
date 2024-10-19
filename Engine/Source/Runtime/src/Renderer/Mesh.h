#pragma once
#include "Heap.h"
#include "Buffer.h"
#include "MeshDescriptor.h"
#include "Assets/Asset.h"

namespace Gleam {

class Mesh : public Asset
{
public:
    
    Mesh(const MeshDescriptor& mesh);
	
	virtual ~Mesh() = default;
    
    virtual void Release() override;
    
    const Buffer& GetPositionBuffer() const;
    
    const Buffer& GetInterleavedBuffer() const;
    
    const Buffer& GetIndexBuffer() const;
    
    const TArray<SubmeshDescriptor>& GetSubmeshes() const;
    
protected:
    
    Heap mHeap;
    Buffer mIndexBuffer;
    Buffer mPositionBuffer;
    Buffer mInterleavedBuffer;
    TArray<SubmeshDescriptor> mSubmeshes;
};

} // namespace Gleam
