#pragma once
#include "Heap.h"
#include "Buffer.h"

namespace Gleam {

struct MeshData;
struct InterleavedMeshVertex;

class MeshBuffer final
{
public:
    
    MeshBuffer(const MeshData& mesh);

	MeshBuffer(const TArray<MeshData>& meshes);
    
    void Dispose();
    
    const Buffer& GetPositionBuffer() const;
    
    const Buffer& GetInterleavedBuffer() const;
    
    const Buffer& GetIndexBuffer() const;
    
private:
    
    MeshBuffer(const TArray<Vector3>& positions, const TArray<InterleavedMeshVertex>& interleavedVertices, const TArray<uint32_t>& indices);

	Heap mHeap;
	Buffer mIndexBuffer;
	Buffer mPositionBuffer;
	Buffer mInterleavedBuffer;
    
};

} // namespace Gleam
