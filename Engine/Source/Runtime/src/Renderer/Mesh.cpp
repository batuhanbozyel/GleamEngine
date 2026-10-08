#include "gpch.h"
#include "Mesh.h"

#include "Core/Engine.h"
#include "Core/Globals.h"

#include "Assets/AssetManager.h"

#include "Renderer/RenderSystem.h"
#include "Renderer/GraphicsDevice.h"
#include "Renderer/CopyCommandBuffer.h"

using namespace Gleam;

Mesh::Mesh(const AssetReference& reference, const AssetHeader& header, const MeshDescriptor& descriptor)
	: Asset(reference, header)
	, mDescriptor(descriptor)
	, mLods(descriptor.lods.size())
{
	GLEAM_ASSERT(mDescriptor.lods.size() > 0, "Mesh has no LODs: {0}", descriptor.name);
	for (uint32_t lod = 0; lod < mLods.size(); ++lod)
	{
		mLods[lod].blases.resize(mDescriptor.lods[lod].submeshes.size());
	}
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

		for (auto& blas : lod.blases)
		{
			if (blas.IsValid())
			{
				device->Dispose(blas);
			}
		}
	}
}

void Mesh::RequestLod(uint32_t lod)
{
	GLEAM_ASSERT(lod < mLods.size(), "Mesh LOD {0} is out of range for: {1}", lod, GetName());

	auto& lodData = mLods[lod].buffer;
	if (not lodData.IsValid())
	{
		static auto renderSystem = Globals::Engine->GetSubsystem<RenderSystem>();
		static auto assetManager = Globals::GameInstance->GetSubsystem<AssetManager>();

		auto storage = assetManager->GetStorage();
		const auto& lodDesc = mDescriptor.lods[lod];
		const auto blob = FindBlob<MeshLodDescriptor>(lodDesc.blobSlot, AssetPlatform::Common, AssetBackend::Common);
		if (blob == nullptr)
		{
			return;
		}

		BufferDescriptor bufferDesc;
		bufferDesc.name = "Mesh: " + mDescriptor.name;
		bufferDesc.size = blob->range.size;
		lodData = renderSystem->GetDevice()->CreateBuffer(renderSystem->GetAllocator(), bufferDesc);

		auto cmd = renderSystem->GetCopyCommandBuffer();
		cmd->Commit(lodData, storage->GetAssetFile(GetReference()), GetBlobRange(*blob), 0);
	}
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

const BoundingBox& Mesh::GetBounds() const
{
	return mDescriptor.bounds;
}

const Buffer& Mesh::GetBuffer(uint32_t lod) const
{
	return mLods[lod].buffer;
}

const BufferRange& Mesh::GetPositions(uint32_t lod) const
{
	return mDescriptor.lods[lod].positions;
}

const BufferRange& Mesh::GetInterleavedVertices(uint32_t lod) const
{
	return mDescriptor.lods[lod].interleavedVertices;
}

const BufferRange& Mesh::GetIndices(uint32_t lod) const
{
	return mDescriptor.lods[lod].indices;
}

const BufferRange& Mesh::GetMeshlets(uint32_t lod) const
{
	return mDescriptor.lods[lod].meshlets;
}

const BufferRange& Mesh::GetMeshletVertices(uint32_t lod) const
{
	return mDescriptor.lods[lod].meshletVertices;
}

const BufferRange& Mesh::GetMeshletTriangleIndices(uint32_t lod) const
{
	return mDescriptor.lods[lod].meshletTriangleIndices;
}

const TArray<SubmeshDescriptor>& Mesh::GetSubmeshes(uint32_t lod) const
{
	return mDescriptor.lods[lod].submeshes;
}

const SubmeshDescriptor& Mesh::GetSubmesh(uint32_t lod, uint32_t index) const
{
	return mDescriptor.lods[lod].submeshes[index];
}

const BottomLevelAccelerationStructure& Mesh::GetBLAS(uint32_t lod, uint32_t submesh) const
{
	return mLods[lod].blases[submesh];
}
