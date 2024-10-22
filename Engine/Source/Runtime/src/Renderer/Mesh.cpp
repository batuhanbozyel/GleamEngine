#include "gpch.h"
#include "Mesh.h"

#include "Core/Engine.h"
#include "Core/Globals.h"
#include "Renderer/RenderSystem.h"

using namespace Gleam;

Mesh::Mesh(const MeshDescriptor& mesh)
    : mSubmeshes(mesh.submeshes)
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
		renderSystem->GetDevice()->GetUploadManager()->CommitUpload(mPositionBuffer, mesh.positions.data(), positionSize);
		renderSystem->GetDevice()->GetUploadManager()->CommitUpload(mInterleavedBuffer, mesh.interleavedVertices.data(), interleavedSize);
		renderSystem->GetDevice()->GetUploadManager()->CommitUpload(mIndexBuffer, mesh.indices.data(), indexSize);
	}
}

void Mesh::Release()
{
	static auto renderSystem = Globals::Engine->GetSubsystem<RenderSystem>();
	auto device = renderSystem->GetDevice();
	device->ReleaseBuffer(mPositionBuffer);
	device->ReleaseBuffer(mInterleavedBuffer);
	device->ReleaseBuffer(mIndexBuffer);
	device->ReleaseHeap(mHeap);
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

const TArray<SubmeshDescriptor>& Mesh::GetSubmeshes() const
{
    return mSubmeshes;
}
