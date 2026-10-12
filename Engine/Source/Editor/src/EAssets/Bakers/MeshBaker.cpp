#include "MeshBaker.h"
#include "EAssets/AssetRegistry.h"
#include "EAssets/AssetWriter.h"
#include "EAssets/Cookers/PhysicsCooker.h"

#include "Assets/Asset.h"

using namespace GEditor;

MeshBaker::MeshBaker(MeshData&& mesh)
	: mMesh(eastl::move(mesh))
{

}

void MeshBaker::Bake(const Gleam::Path& directory, const AssetItem& item) const
{
	Gleam::MeshDescriptor descriptor;
	descriptor.name = mMesh.name;
	descriptor.bounds = mMesh.aabb;
	descriptor.lods.resize(mMesh.lods.size());

	BinaryAssetWriter writer;
	for (uint32_t i = 0; i < mMesh.lods.size(); ++i)
	{
		const auto& lod = mMesh.lods[i];
		auto& lodDesc = descriptor.lods[i];

		lodDesc.indices = lod.indices;
		lodDesc.positions = lod.positions;
		lodDesc.interleavedVertices = lod.interleavedVertices;
		lodDesc.meshlets = lod.meshlets;
		lodDesc.meshletVertices = lod.meshletVertices;
		lodDesc.meshletTriangleIndices = lod.meshletTriangleIndices;
		lodDesc.submeshes = lod.submeshes;

		lodDesc.blobSlot = writer.AddBlob<Gleam::MeshLodDescriptor>(lod.buffer.data,
																	lod.buffer.size,
																	Gleam::AssetPlatform::Common,
																	Gleam::AssetBackend::Common);
	}

	const auto cooker = PhysicsCooker::Create();
	for (const auto& hull : mMesh.convexHulls)
	{
		auto blob = cooker->CookConvexHull(hull.positions);
		if (blob.size > 0)
		{
			auto& convexHullDesc = descriptor.convexHulls.emplace_back();
			convexHullDesc.blobSlot = writer.AddBlob<Gleam::ConvexHullDescriptor>(eastl::move(blob),
																				  Gleam::AssetPlatform::Common,
																				  cooker->GetBackend());
		}
		else
		{
			GLEAM_WARN("Failed to cook convex hull for mesh: {0}", mMesh.name);
		}
	}

	for (const auto& triangleMesh : mMesh.triangleMeshes)
	{
		auto blob = cooker->CookTriangleMesh(triangleMesh);
		if (blob.size > 0)
		{
			auto& triangleMeshDesc = descriptor.triangleMeshes.emplace_back();
			triangleMeshDesc.blobSlot = writer.AddBlob<Gleam::TriangleMeshDescriptor>(eastl::move(blob),
																					  Gleam::AssetPlatform::Common,
																					  cooker->GetBackend());
		}
		else
		{
			GLEAM_WARN("Failed to cook triangle mesh for mesh: {0}", mMesh.name);
		}
	}
	writer.Write(directory, item, descriptor);
}

Gleam::TString MeshBaker::Name() const
{
	return mMesh.name;
}

Gleam::Guid MeshBaker::TypeGuid() const
{
	return Gleam::Reflection::GetClass<Gleam::MeshDescriptor>().Guid();
}
