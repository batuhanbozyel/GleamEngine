#pragma once
#include "Buffer.h"
#include "MeshDescriptor.h"
#include "AccelerationStructure.h"
#include "Assets/Asset.h"

namespace Gleam {

class RayTracingScene;

class Mesh : public Asset
{
	friend class RayTracingScene;
public:

	Mesh(const AssetReference& reference, const AssetHeader& header, const MeshDescriptor& descriptor);

	~Mesh();

	void RequestLod(uint32_t lod);

	uint32_t GetLodCount() const;

	bool IsLodResident(uint32_t lod) const;

	const BoundingBox& GetBounds() const;

	const Buffer& GetBuffer(uint32_t lod) const;

	const BufferRange& GetPositions(uint32_t lod) const;

	const BufferRange& GetInterleavedVertices(uint32_t lod) const;

	const BufferRange& GetIndices(uint32_t lod) const;

	const BufferRange& GetMeshlets(uint32_t lod) const;

	const BufferRange& GetMeshletVertices(uint32_t lod) const;

	const BufferRange& GetMeshletTriangleIndices(uint32_t lod) const;

	const TArray<SubmeshDescriptor>& GetSubmeshes(uint32_t lod) const;

	const SubmeshDescriptor& GetSubmesh(uint32_t lod, uint32_t index) const;

	const BottomLevelAccelerationStructure& GetBLAS(uint32_t lod, uint32_t submesh) const;

	const TArray<ConvexHullDescriptor>& GetConvexHulls() const;

	const TArray<TriangleMeshDescriptor>& GetTriangleMeshes() const;

protected:

	struct LodResources
	{
		Buffer buffer;
		TArray<BottomLevelAccelerationStructure> blases;
	};

	MeshDescriptor mDescriptor;
	TArray<LodResources> mLods;
};

} // namespace Gleam
