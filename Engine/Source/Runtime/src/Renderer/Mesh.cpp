#include "gpch.h"
#include "Mesh.h"

#include "Core/Engine.h"
#include "Core/Globals.h"
#include "Renderer/RenderSystem.h"

using namespace Gleam;

Mesh::Mesh(const MeshDescriptor& mesh)
    : mSubmeshDescriptors(mesh.submeshes)
{
    static auto renderSystem = Globals::Engine->GetSubsystem<RenderSystem>();
    
    size_t positionSize = mesh.positions.size() * sizeof(Float3);
    size_t interleavedSize = mesh.interleavedVertices.size() * sizeof(InterleavedMeshVertex);
    size_t indexSize = mesh.indices.size() * sizeof(uint32_t);

    HeapDescriptor heapDesc;
    heapDesc.name = mesh.name;
    heapDesc.memoryType = MemoryType::GPU;
    heapDesc.size = positionSize + interleavedSize + indexSize;
    auto memoryRequirements = renderSystem->GetDevice()->QueryMemoryRequirements(heapDesc);
    
    size_t positionBufferSize = Utils::AlignUp(positionSize, memoryRequirements.alignment);
    size_t interleavedBufferSize = Utils::AlignUp(interleavedSize, memoryRequirements.alignment);
    size_t indexBufferSize = Utils::AlignUp(indexSize, memoryRequirements.alignment);

    heapDesc.size = positionBufferSize + interleavedBufferSize + indexBufferSize;
    mHeap = renderSystem->GetDevice()->CreateHeap(heapDesc);

    BufferDescriptor bufferDesc;
    bufferDesc.name = "Positions";
    bufferDesc.size = positionBufferSize;
    mPositionBuffer = mHeap.CreateBuffer(bufferDesc);
    
    bufferDesc.name = "InterleavedData";
    bufferDesc.size = interleavedBufferSize;
    mInterleavedBuffer = mHeap.CreateBuffer(bufferDesc);
    
    bufferDesc.name = "Indices";
    bufferDesc.size = indexBufferSize;
    mIndexBuffer = mHeap.CreateBuffer(bufferDesc);

    // Send mesh data to buffers
    {
        heapDesc.name += "::StagingHeap";
        heapDesc.memoryType = MemoryType::CPU;
        Heap heap = renderSystem->GetDevice()->CreateHeap(heapDesc);
        
        bufferDesc.name = "StagingBuffer";
        bufferDesc.size = heapDesc.size;
        Buffer stagingBuffer = heap.CreateBuffer(bufferDesc);

        CommandBuffer commandBuffer(renderSystem->GetDevice());
        commandBuffer.Begin();

        size_t offset = 0;
        commandBuffer.SetBufferData(stagingBuffer, mesh.positions.data(), positionSize, offset);
        commandBuffer.CopyBuffer(stagingBuffer, mPositionBuffer, positionSize, offset, 0);

        offset += positionBufferSize;
        commandBuffer.SetBufferData(stagingBuffer, mesh.interleavedVertices.data(), interleavedSize, offset);
        commandBuffer.CopyBuffer(stagingBuffer, mInterleavedBuffer, interleavedSize, offset, 0);

        offset += interleavedBufferSize;
        commandBuffer.SetBufferData(stagingBuffer, mesh.indices.data(), indexSize, offset);
        commandBuffer.CopyBuffer(stagingBuffer, mIndexBuffer, indexSize, offset, 0);

        commandBuffer.End();
        commandBuffer.Commit();
        commandBuffer.WaitUntilCompleted();

        renderSystem->GetDevice()->Dispose(stagingBuffer);
        renderSystem->GetDevice()->Dispose(heap);
    }
}

void Mesh::Dispose()
{
    static auto renderSystem = Globals::Engine->GetSubsystem<RenderSystem>();
    renderSystem->GetDevice()->Dispose(mPositionBuffer);
    renderSystem->GetDevice()->Dispose(mInterleavedBuffer);
    renderSystem->GetDevice()->Dispose(mIndexBuffer);
    renderSystem->GetDevice()->Dispose(mHeap);
}

const Buffer& Mesh::GetPositionBuffer() const
{
    return mPositionBuffer;
}

const Buffer& Mesh::GetInterleavedBuffer() const
{
    return mInterleavedBuffer;
}

const Buffer& Mesh::GetIndexBuffer() const
{
    return mIndexBuffer;
}

uint32_t Mesh::GetSubmeshCount() const
{
    return static_cast<uint32_t>(mSubmeshDescriptors.size());
}

const TArray<SubmeshDescriptor>& Mesh::GetSubmeshDescriptors() const
{
    return mSubmeshDescriptors;
}
