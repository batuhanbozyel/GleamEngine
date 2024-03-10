#include "gpch.h"
#include "Mesh.h"
#include "MeshBuffer.h"
#include "Core/Application.h"
#include "Renderer/RenderSystem.h"

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
        for (uint32_t i = 0; i < mesh.positions.size(); i++)
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
    static auto renderSystem = GameInstance->GetSubsystem<RenderSystem>();

    size_t positionSize = positions.size() * sizeof(Vector3);
	size_t interleavedSize = interleavedVertices.size() * sizeof(InterleavedMeshVertex);
	size_t indexSize = indices.size() * sizeof(uint32_t);

    HeapDescriptor heapDesc;
    heapDesc.memoryType = MemoryType::GPU;
	auto memoryRequirements = renderSystem->GetDevice()->QueryMemoryRequirements(heapDesc);
	heapDesc.size = Utils::AlignUp(positionSize, memoryRequirements.alignment) +
					Utils::AlignUp(interleavedSize, memoryRequirements.alignment) +
					Utils::AlignUp(indexSize, memoryRequirements.alignment);
    mHeap = renderSystem->GetDevice()->CreateHeap(heapDesc);

    mPositionBuffer = mHeap.CreateBuffer(positionSize);
    mInterleavedBuffer = mHeap.CreateBuffer(interleavedSize);
    mIndexBuffer = mHeap.CreateBuffer(indexSize);

    // Send mesh data to buffers
    {
        heapDesc.memoryType = MemoryType::CPU;
        Heap heap = renderSystem->GetDevice()->CreateHeap(heapDesc);
        Buffer stagingBuffer = heap.CreateBuffer(heapDesc.size);

        CommandBuffer commandBuffer(renderSystem->GetDevice());
        commandBuffer.Begin();

        size_t offset = 0;
        commandBuffer.SetBufferData(stagingBuffer, positions.data(), positionSize, offset);
        commandBuffer.CopyBuffer(stagingBuffer, mPositionBuffer, positionSize, offset, 0);

        offset += positionSize;
        commandBuffer.SetBufferData(stagingBuffer, interleavedVertices.data(), interleavedSize, offset);
        commandBuffer.CopyBuffer(stagingBuffer, mInterleavedBuffer, interleavedSize, offset, 0);

        offset += interleavedSize;
        commandBuffer.SetBufferData(stagingBuffer, indices.data(), indexSize, offset);
        commandBuffer.CopyBuffer(stagingBuffer, mIndexBuffer, indexSize, offset, 0);

        commandBuffer.End();
        commandBuffer.Commit();
		commandBuffer.WaitUntilCompleted();

        renderSystem->GetDevice()->Dispose(stagingBuffer);
        renderSystem->GetDevice()->Dispose(heap);
    }
}

MeshBuffer::MeshBuffer(const MeshData& mesh)
    : MeshBuffer(mesh.positions, GetInterleavedVertices(mesh), mesh.indices)
{
    
}

MeshBuffer::MeshBuffer(const TArray<MeshData>& meshes)
    : MeshBuffer(BatchPositionData(meshes), BatchInterleavedData(meshes), BatchIndexData(meshes))
{
    
}

void MeshBuffer::Dispose()
{
    static auto renderSystem = GameInstance->GetSubsystem<RenderSystem>();
    renderSystem->GetDevice()->Dispose(mPositionBuffer);
    renderSystem->GetDevice()->Dispose(mInterleavedBuffer);
    renderSystem->GetDevice()->Dispose(mIndexBuffer);
    renderSystem->GetDevice()->Dispose(mHeap);
}

const Buffer& MeshBuffer::GetPositionBuffer() const
{
    return mPositionBuffer;
}

const Buffer& MeshBuffer::GetInterleavedBuffer() const
{
    return mInterleavedBuffer;
}

const Buffer& MeshBuffer::GetIndexBuffer() const
{
    return mIndexBuffer;
}
