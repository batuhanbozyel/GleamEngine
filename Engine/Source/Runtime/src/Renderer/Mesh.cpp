#include "gpch.h"
#include "Mesh.h"

#include "Core/Engine.h"
#include "Core/Globals.h"

#include "Renderer/RenderSystem.h"
#include "Renderer/GraphicsDevice.h"
#include "Renderer/CopyCommandBuffer.h"

using namespace Gleam;

Mesh::Mesh(const MeshDescriptor& descriptor)
    : Asset(descriptor.name)
	, mSubmeshes(descriptor.submeshes)
	, mBLASes(descriptor.submeshes.size())
{
    static auto renderSystem = Globals::Engine->GetSubsystem<RenderSystem>();
	auto device = renderSystem->GetDevice();

    BufferDescriptor bufferDesc;
    bufferDesc.name = "Mesh: " + descriptor.name + " Positions";
    bufferDesc.size = descriptor.positions.size;
    mPositionBuffer = device->CreateBuffer(renderSystem->GetAllocator(), bufferDesc);

    bufferDesc.name = "Mesh: " + descriptor.name + " InterleavedData";
    bufferDesc.size = descriptor.interleavedVertices.size;
    mInterleavedBuffer = device->CreateBuffer(renderSystem->GetAllocator(), bufferDesc);

    bufferDesc.name = "Mesh: " + descriptor.name + " Indices";
    bufferDesc.size = descriptor.indices.size;
    mIndexBuffer = device->CreateBuffer(renderSystem->GetAllocator(), bufferDesc);

	bufferDesc.name = "Mesh: " + descriptor.name + " MeshletVertices";
	bufferDesc.size = descriptor.meshletVertices.size;
	mMeshletVertexBuffer = device->CreateBuffer(renderSystem->GetAllocator(), bufferDesc);

	bufferDesc.name = "Mesh: " + descriptor.name + " MeshletTriangles";
	bufferDesc.size = descriptor.meshletTriangleIndices.size;
	mMeshletTriangleBuffer = device->CreateBuffer(renderSystem->GetAllocator(), bufferDesc);

	bufferDesc.name = "Mesh: " + descriptor.name + " Meshlets";
	bufferDesc.size = descriptor.meshlets.size;
	mMeshletsBuffer = device->CreateBuffer(renderSystem->GetAllocator(), bufferDesc);

    // Send mesh data to buffers
	{
		auto cmd = renderSystem->GetCopyCommandBuffer();
		cmd->Commit(mPositionBuffer, OffsetPointer(descriptor.buffer.data, descriptor.positions.offset), descriptor.positions.size, 0);
		cmd->Commit(mInterleavedBuffer, OffsetPointer(descriptor.buffer.data, descriptor.interleavedVertices.offset), descriptor.interleavedVertices.size, 0);
		cmd->Commit(mIndexBuffer, OffsetPointer(descriptor.buffer.data, descriptor.indices.offset), descriptor.indices.size, 0);
		cmd->Commit(mMeshletVertexBuffer, OffsetPointer(descriptor.buffer.data, descriptor.meshletVertices.offset), descriptor.meshletVertices.size, 0);
		cmd->Commit(mMeshletTriangleBuffer, OffsetPointer(descriptor.buffer.data, descriptor.meshletTriangleIndices.offset), descriptor.meshletTriangleIndices.size, 0);
		cmd->Commit(mMeshletsBuffer, OffsetPointer(descriptor.buffer.data, descriptor.meshlets.offset), descriptor.meshlets.size, 0);
	}
}

Mesh::~Mesh()
{
	static auto renderSystem = Globals::Engine->GetSubsystem<RenderSystem>();
	auto device = renderSystem->GetDevice();
	device->Dispose(renderSystem->GetAllocator(), mPositionBuffer, BarrierStage::None);
	device->Dispose(renderSystem->GetAllocator(), mInterleavedBuffer, BarrierStage::None);
	device->Dispose(renderSystem->GetAllocator(), mIndexBuffer, BarrierStage::None);
	device->Dispose(renderSystem->GetAllocator(), mMeshletVertexBuffer, BarrierStage::None);
	device->Dispose(renderSystem->GetAllocator(), mMeshletTriangleBuffer, BarrierStage::None);
	device->Dispose(renderSystem->GetAllocator(), mMeshletsBuffer, BarrierStage::None);

	for (auto& blas : mBLASes)
	{
		if (blas.IsValid())
		{
			device->Dispose(blas);
		}
	}
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

const Buffer& Mesh::GetMeshletVertexBuffer() const
{
	return mMeshletVertexBuffer;
}

const Buffer& Mesh::GetMeshletTriangleBuffer() const
{
	return mMeshletTriangleBuffer;
}

const Buffer& Mesh::GetMeshletsBuffer() const
{
	return mMeshletsBuffer;
}

const TArray<SubmeshDescriptor>& Mesh::GetSubmeshes() const
{
    return mSubmeshes;
}

const SubmeshDescriptor& Mesh::GetSubmesh(uint32_t index) const
{
	return mSubmeshes[index];
}

const BottomLevelAccelerationStructure& Mesh::GetBLAS(uint32_t submesh) const
{
	return mBLASes[submesh];
}
