#include "gpch.h"
#include "MeshBuffer.h"
#include "Assets/Model.h"

using namespace Gleam;

static TArray<Vector3> BatchPositionData(const TArray<MeshData>& meshes)
{
    size_t count = 0;
    for (const auto& mesh : meshes)
    {
        count += mesh.positions.size();
    }
    
    size_t offset = 0;
    TArray<Vector3> positionData(count);
    for (const auto& mesh : meshes)
    {
        for (uint32_t i = 0; i < mesh.normals.size(); i++)
        {
            positionData[i + offset] = mesh.positions[i];
        }
        offset += mesh.positions.size();
    }
    return positionData;
}

static TArray<InterleavedMeshVertex> BatchInterleavedData(const TArray<MeshData>& meshes)
{
    size_t count = 0;
    for (const auto& mesh : meshes)
    {
        count += mesh.normals.size();
    }
    
    size_t offset = 0;
    TArray<InterleavedMeshVertex> interleavedData(count);
    for (const auto& mesh : meshes)
    {
        for (uint32_t i = 0; i < mesh.normals.size(); i++)
        {
            interleavedData[i + offset].normal = mesh.normals[i];
            interleavedData[i + offset].texCoord = mesh.texCoords[i];
        }
        offset += mesh.normals.size();
    }
    return interleavedData;
}

static TArray<uint32_t> BatchIndexData(const TArray<MeshData>& meshes)
{
    size_t count = 0;
    for (const auto& mesh : meshes)
    {
        count += mesh.indices.size();
    }
    
    size_t offset = 0;
    TArray<uint32_t> indexData(count);
    for (const auto& mesh : meshes)
    {
        for (uint32_t i = 0; i < mesh.indices.size(); i++)
        {
            indexData[i + offset] = mesh.indices[i];
        }
        offset += mesh.indices.size();
    }
    return indexData;
}

static TArray<InterleavedMeshVertex> GetInterleavedVertices(const MeshData& mesh)
{
    TArray<InterleavedMeshVertex> interleavedVertices(mesh.normals.size());
    for (uint32_t i = 0; i < mesh.normals.size(); i++)
    {
        interleavedVertices[i].normal = mesh.normals[i];
        interleavedVertices[i].texCoord = mesh.texCoords[i];
    }
    return interleavedVertices;
}

MeshBuffer::MeshBuffer(const TArray<Vector3>& positions, const TArray<InterleavedMeshVertex>& interleavedVertices, const TArray<uint32_t>& indices)
{
    BufferDescriptor descriptor;
    descriptor.size = positions.size() * sizeof(Vector3);
    descriptor.usage = BufferUsage::VertexBuffer;
    mPositionBuffer = CreateScope<Buffer>(positions.data(), descriptor);
    
    
    descriptor.size = interleavedVertices.size() * sizeof(InterleavedMeshVertex);
    mInterleavedBuffer = CreateScope<Buffer>(interleavedVertices.data(), descriptor);
    
    descriptor.size = indices.size() * sizeof(uint32_t);
    descriptor.usage = BufferUsage::IndexBuffer;
    mIndexBuffer = CreateScope<Buffer>(indices.data(), descriptor);
}

MeshBuffer::MeshBuffer(const MeshData& mesh)
    : MeshBuffer(mesh.positions, GetInterleavedVertices(mesh), mesh.indices)
{
    
}

MeshBuffer::MeshBuffer(const TArray<MeshData>& meshes)
    : MeshBuffer(BatchPositionData(meshes), BatchInterleavedData(meshes), BatchIndexData(meshes))
{
    
}

const Buffer* MeshBuffer::GetPositionBuffer() const
{
    return mPositionBuffer.get();
}

const Buffer* MeshBuffer::GetInterleavedBuffer() const
{
    return mInterleavedBuffer.get();
}

const Buffer* MeshBuffer::GetIndexBuffer() const
{
    return mIndexBuffer.get();
}
