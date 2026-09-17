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

	uint32_t GetActiveLod() const;

	uint32_t GetLodCount() const;

	bool IsLodResident(uint32_t lod) const;

	const Buffer& GetBuffer() const;

	const BufferRange& GetPositions() const;

	const BufferRange& GetInterleavedVertices() const;

	const BufferRange& GetIndices() const;

	const BufferRange& GetMeshlets() const;

	const BufferRange& GetMeshletVertices() const;

	const BufferRange& GetMeshletTriangleIndices() const;

	const TArray<SubmeshDescriptor>& GetSubmeshes() const;

	const SubmeshDescriptor& GetSubmesh(uint32_t index) const;

	const BottomLevelAccelerationStructure& GetBLAS(uint32_t submesh) const;

protected:

	MeshDescriptor mDescriptor;
	TArray<Buffer> mLods;
	uint32_t mActiveLod = 0;
	TArray<BottomLevelAccelerationStructure> mBLASes;
};

} // namespace Gleam
