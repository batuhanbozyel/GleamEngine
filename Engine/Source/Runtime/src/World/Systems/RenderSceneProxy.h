#pragma once
#include "World/WorldSubsystem.h"
#include "World/Entity.h"
#include "Renderer/Buffer.h"
#include "Renderer/MeshDrawList.h"
#include "Renderer/Shaders/ShaderTypes.h"
#include "Assets/AssetReference.h"
#include "Container/Hash.h"

#include <functional>

namespace Gleam {

class Mesh;
class Entity;
class Material;
class MaterialInstance;
class CommandBuffer;
class GPUAllocator;
struct MeshRenderer;
struct Camera;
struct Transform;

struct MeshDraw
{
	uint32_t record = 0;
	uint32_t instance = 0;
};

struct MeshBatch
{
	Material* material = nullptr;
	TArray<MeshDraw> draws;
	uint32_t instanceOffset = 0;
	uint32_t batchIndex = 0;
};

struct MeshInstanceRecord
{
	MaterialInstance* material = nullptr;
	uint32_t submeshIndex = 0;
	uint32_t batch = 0;
	uint32_t draw = 0;
};

struct MeshEntityRecord
{
	EntityHandle entity = InvalidEntity;

	TArray<AssetReference> acquired;
	TArray<MeshInstanceRecord> instances;

	Float4x4 transform = Float4x4::identity;
	Float4x4 prevTransform = Float4x4::identity;

	Float3 worldCenter = Float3::zero;
	float worldRadius = 0.0f;

	Mesh* mesh = nullptr;
	uint32_t lod = 0;
};

class RenderSceneProxy : public WorldSubsystem
{
    using BatchFn = std::function<void(const MeshBatch&)>;

	static constexpr uint32_t MaxMeshInstances = MAX_MESH_INSTANCES;
	static_assert(MaxMeshInstances <= VISIBILITY_INSTANCE_MASK, "MaxMeshInstances exceeds the visibility buffer instance ID bit budget.");
public:
    
	virtual void Initialize(World* world) override;

	void Update(const World* world, EntityHandle camera);

	void BuildInstanceBuffer(const CommandBuffer* cmd, GPUAllocator* allocator);

	void ReleaseInstanceBuffer(GPUAllocator* allocator);

	void Cull(const Frustum& frustum, MeshDrawList& drawList) const;

	virtual void Shutdown(World* world) override;
    
    void ForEach(BatchFn&& fn) const;

	template<typename Fn>
	void ForEachDraw(const MeshBatch& batch, Fn&& fn) const
	{
		for (uint32_t d = 0; d < batch.draws.size(); ++d)
		{
			const auto& draw = batch.draws[d];
			const auto& record = mRecords[draw.record];
			fn(batch.instanceOffset + d, record, record.instances[draw.instance]);
		}
	}

	template<typename Fn>
	void ForEachDraw(const MeshBatch& batch, const MeshDrawList& drawList, Fn&& fn) const
	{
		const auto& range = drawList.batches[batch.batchIndex];
		for (uint32_t i = range.first; i < range.first + range.count; ++i)
		{
			const uint32_t d = drawList.draws[i];
			const auto& draw = batch.draws[d];
			const auto& record = mRecords[draw.record];
			fn(batch.instanceOffset + d, record, record.instances[draw.instance]);
		}
	}

	const Buffer& GetInstanceBuffer() const
	{
		return mInstanceBuffer;
	}

	uint32_t GetInstanceCount() const
	{
		return mTotalInstances;
	}

	uint32_t GetBatchCount() const
	{
		return mNumBatches;
	}

private:

	void BuildRecord(const EntityManager& entityManager, EntityHandle entity, const MeshRenderer& meshRenderer);

	void RemoveRecord(EntityHandle entity);

	void OnMeshRendererRemoved(EntityHandle entity);

	void ReleaseAcquired(TArray<AssetReference>& acquired);

	void AddDraws(uint32_t recordIndex);

	void RemoveDraws(uint32_t recordIndex);

	Buffer mInstanceBuffer = {};

	uint32_t mNumBatches = 0;
	uint32_t mTotalInstances = 0;
	TArray<MeshBatch> mBatches;
	HashMap<AssetReference, uint32_t> mBatchLookup;

	ChangeCursor mChangeCursor;
	ComponentCallback mMeshRendererRemoved;

	TArray<MeshEntityRecord> mRecords;
	HashMap<uint32_t, uint32_t> mRecordLookup;

};

} // namespace Gleam
