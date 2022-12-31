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

MeshBuffer::MeshBuffer(const TArray<MeshData>& meshes)
    : mPositionBuffer(PreparePositionData(meshes)), mInterleavedBuffer(PrepareInterleavedData(meshes)), mIndexBuffer(PrepareIndexData(meshes))
{
    
}

TArray<Vector3> MeshBuffer::PreparePositionData(const TArray<MeshData>& meshes)
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

TArray<InterleavedMeshVertex> MeshBuffer::PrepareInterleavedData(const TArray<MeshData>& meshes)
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

TArray<uint32_t> MeshBuffer::PrepareIndexData(const TArray<MeshData>& meshes)
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
