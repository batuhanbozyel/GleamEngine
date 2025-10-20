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
    mPositionBuffer = mHeap.Allocate(bufferDesc);
    
    bufferDesc.name = "InterleavedData";
    bufferDesc.size = interleavedBufferSize;
    mInterleavedBuffer = mHeap.Allocate(bufferDesc);
    
    bufferDesc.name = "Indices";
    bufferDesc.size = indexBufferSize;
    mIndexBuffer = mHeap.Allocate(bufferDesc);

    // Send mesh data to buffers
	{
		renderSystem->GetUploadManager()->Commit(mPositionBuffer, mesh.positions.data(), positionSize);
		renderSystem->GetUploadManager()->Commit(mInterleavedBuffer, mesh.interleavedVertices.data(), interleavedSize);
		renderSystem->GetUploadManager()->Commit(mIndexBuffer, mesh.indices.data(), indexSize);
	}
}

Mesh::~Mesh()
{
	static auto renderSystem = Globals::Engine->GetSubsystem<RenderSystem>();
	auto device = renderSystem->GetDevice();
	mHeap.Free(mPositionBuffer);
	mHeap.Free(mInterleavedBuffer);
	mHeap.Free(mIndexBuffer);
	device->Dispose(mHeap);
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
