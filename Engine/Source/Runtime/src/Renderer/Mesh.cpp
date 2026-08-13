#include "gpch.h"
#include "Mesh.h"

#include "Core/Engine.h"
#include "Core/Globals.h"

#include "Assets/AssetManager.h"

#include "Renderer/RenderSystem.h"
#include "Renderer/GraphicsDevice.h"
#include "Renderer/CopyCommandBuffer.h"

using namespace Gleam;

Mesh::Mesh(const AssetReference& reference, const MeshDescriptor& descriptor)
	: Asset(reference, descriptor.name)
	, mDescriptor(descriptor)
	, mLods(descriptor.lods.size())
{
	if (mDescriptor.lods.empty())
	{
		GLEAM_CORE_ERROR("Mesh has no LODs: {0}", descriptor.name);
		GLEAM_ASSERT(false);
		return;
	}

	mBLASes.resize(mDescriptor.lods[0].submeshes.size());
	RequestLod(0);
}

Mesh::~Mesh()
{
	static auto renderSystem = Globals::Engine->GetSubsystem<RenderSystem>();
	auto device = renderSystem->GetDevice();

	for (auto& lod : mLods)
	{
		if (lod.buffer.IsValid())
		{
			device->Dispose(renderSystem->GetAllocator(), lod.buffer, BarrierStage::None);
		}
	}

	for (auto& blas : mBLASes)
	{
		if (blas.IsValid())
		{
			device->Dispose(blas);
		}
	}
}

void Mesh::RequestLod(uint32_t lod)
{
	if (lod >= mLods.size())
	{
		GLEAM_CORE_ERROR("Mesh LOD {0} is out of range for: {1}", lod, GetName());
		GLEAM_ASSERT(false);
		return;
	}

	const auto& lodDesc = mDescriptor.lods[lod];
	auto& lodData = mLods[lod];

	static auto assetManager = Globals::GameInstance->GetSubsystem<AssetManager>();
	auto storage = assetManager->GetStorage();
	const auto& chunkTable = storage->ReadChunkTable(GetReference());

	const uint32_t chunkIndices[] = {
		lodDesc.indicesChunk,
		lodDesc.positionsChunk,
		lodDesc.interleavedChunk,
		lodDesc.meshletsChunk,
		lodDesc.meshletVerticesChunk,
		lodDesc.meshletTrianglesChunk
	};

	BufferRange* ranges[] = {
		&lodData.indices,
		&lodData.positions,
		&lodData.interleavedVertices,
		&lodData.meshlets,
		&lodData.meshletVertices,
		&lodData.meshletTriangleIndices
	};

	constexpr uint32_t streamCount = 6;

	uint64_t offset = 0;
	for (uint32_t i = 0; i < streamCount; ++i)
	{
		if (chunkIndices[i] >= chunkTable.chunks.size())
		{
			GLEAM_CORE_ERROR("Mesh chunk {0} is out of range for: {1}", chunkIndices[i], GetName());
			GLEAM_ASSERT(false);
			return;
		}

		ranges[i]->offset = offset;
		ranges[i]->size = chunkTable.chunks[chunkIndices[i]].size;
		offset += ranges[i]->size;
	}

	if (not lodData.buffer.IsValid())
	{
		static auto renderSystem = Globals::Engine->GetSubsystem<RenderSystem>();

		BufferDescriptor bufferDesc;
		bufferDesc.name = "Mesh: " + mDescriptor.name;
		bufferDesc.size = offset;
		lodData.buffer = renderSystem->GetDevice()->CreateBuffer(renderSystem->GetAllocator(), bufferDesc);
	}

	for (uint32_t i = 0; i < streamCount; ++i)
	{
		storage->Enqueue(ChunkReadRequest{
			.asset = GetReference(),
			.chunkIndex = chunkIndices[i],
			.destination = MakeBufferDestination(lodData.buffer, ranges[i]->offset)
		});
	}

	storage->Wait(storage->Submit());
}

void Mesh::SetActiveLod(uint32_t lod)
{
	if (lod >= mLods.size())
	{
		GLEAM_CORE_ERROR("Mesh LOD {0} is out of range for: {1}", lod, GetName());
		GLEAM_ASSERT(false);
		return;
	}
	mActiveLod = lod;
}

uint32_t Mesh::GetActiveLod() const
{
	return mActiveLod;
}

uint32_t Mesh::GetLodCount() const
{
	return static_cast<uint32_t>(mLods.size());
}

bool Mesh::IsLodResident(uint32_t lod) const
{
	if (lod >= mLods.size())
	{
		return false;
	}
	return mLods[lod].buffer.IsValid();
}

const Mesh::MeshLod& Mesh::GetActiveLodData() const
{
	return mLods[mActiveLod];
}

const Buffer& Mesh::GetBuffer() const
{
	return GetActiveLodData().buffer;
}

const BufferRange& Mesh::GetPositions() const
{
	return GetActiveLodData().positions;
}

const BufferRange& Mesh::GetInterleavedVertices() const
{
	return GetActiveLodData().interleavedVertices;
}

const BufferRange& Mesh::GetIndices() const
{
	return GetActiveLodData().indices;
}

const BufferRange& Mesh::GetMeshlets() const
{
	return GetActiveLodData().meshlets;
}

const BufferRange& Mesh::GetMeshletVertices() const
{
	return GetActiveLodData().meshletVertices;
}

const BufferRange& Mesh::GetMeshletTriangleIndices() const
{
	return GetActiveLodData().meshletTriangleIndices;
}

const TArray<SubmeshDescriptor>& Mesh::GetSubmeshes() const
{
	return mDescriptor.lods[mActiveLod].submeshes;
}

const SubmeshDescriptor& Mesh::GetSubmesh(uint32_t index) const
{
	return mDescriptor.lods[mActiveLod].submeshes[index];
}

const BottomLevelAccelerationStructure& Mesh::GetBLAS(uint32_t submesh) const
{
	return mBLASes[submesh];
}
