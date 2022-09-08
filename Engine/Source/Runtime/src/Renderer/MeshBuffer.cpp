#include "gpch.h"
#include "MeshBuffer.h"

using namespace Gleam;

MeshBuffer::MeshBuffer(const MeshData& mesh)
    : mPositionBuffer(mesh.positions), mInterleavedBuffer(mesh.normals.size()), mIndexBuffer(mesh.indices)
{
    TArray<InterleavedMeshVertex> interleavedData(mesh.normals.size());
    for (uint32_t i = 0; i < mesh.normals.size(); i++)
    {
        interleavedData[i].normal = mesh.normals[i];
        interleavedData[i].texCoord = mesh.texCoords[i];
    }
    mInterleavedBuffer.SetData(interleavedData);
}

const VertexBuffer<Vector3>& MeshBuffer::GetPositionBuffer() const
{
    return mPositionBuffer;
}

const VertexBuffer<InterleavedMeshVertex>& MeshBuffer::GetInterleavedBuffer() const
{
    return mInterleavedBuffer;
}

const IndexBuffer<IndexType::UINT32>& MeshBuffer::GetIndexBuffer() const
{
    return mIndexBuffer;
}
