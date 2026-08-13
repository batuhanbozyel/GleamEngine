#include "MeshBaker.h"
#include "EAssets/AssetRegistry.h"
#include "EAssets/AssetContainerWriter.h"

#include "Assets/Asset.h"

using namespace GEditor;

MeshBaker::MeshBaker(MeshLodData&& lod)
{
	mLods.emplace_back(std::move(lod));
}

void MeshBaker::Bake(const Gleam::Path& directory, const AssetItem& item) const
{
	auto filename = Gleam::TWString(item.reference.guid.ToString()) + Gleam::Asset::Extension();

	Gleam::MeshDescriptor descriptor;
	descriptor.name = Filename();
	descriptor.lods.resize(mLods.size());

	auto writer = AssetContainerWriter(TypeGuid(), descriptor.name);
	for (uint32_t i = 0; i < mLods.size(); ++i)
	{
		const auto& lod = mLods[i];
		auto& lodDesc = descriptor.lods[i];

		lodDesc.indicesChunk = writer.AddChunk(lod.indices.data(), lod.indices.size() * sizeof(uint32_t));
		lodDesc.positionsChunk = writer.AddChunk(lod.positions.data(), lod.positions.size() * sizeof(Gleam::Float3));
		lodDesc.interleavedChunk = writer.AddChunk(lod.interleavedVertices.data(), lod.interleavedVertices.size() * sizeof(Gleam::InterleavedMeshVertex));
		lodDesc.meshletsChunk = writer.AddChunk(lod.meshlets.data(), lod.meshlets.size() * sizeof(Gleam::MeshletDescriptor));
		lodDesc.meshletVerticesChunk = writer.AddChunk(lod.meshletVertices.data(), lod.meshletVertices.size() * sizeof(uint32_t));
		lodDesc.meshletTrianglesChunk = writer.AddChunk(lod.meshletTriangleIndices.data(), lod.meshletTriangleIndices.size() * sizeof(uint32_t));

		lodDesc.submeshes = lod.submeshes;
	}

	writer.Write(directory / filename, descriptor);
}

Gleam::TString MeshBaker::Filename() const
{
	return mLods.empty() ? Gleam::TString() : mLods[0].name;
}

Gleam::Guid MeshBaker::TypeGuid() const
{
	return Gleam::Reflection::GetClass<Gleam::MeshDescriptor>().Guid();
}
