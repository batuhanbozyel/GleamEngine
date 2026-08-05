#pragma once
#include "World/WorldSubsystem.h"
#include "World/Entity.h"
#include "Renderer/Buffer.h"
#include "Renderer/Shaders/ShaderTypes.h"
#include "Assets/AssetReference.h"
#include "Container/Hash.h"

#include <functional>

namespace Gleam {

class Mesh;
class Entity;
class Material;
class MaterialInstance;

struct MeshBatch
{
	Material* material = nullptr;
	uint32_t instanceOffset = 0;
	uint32_t numInstances = 0;
	uint32_t batchIndex = 0;
};

struct MeshInstance
{
	Mesh* mesh = nullptr;
	uint32_t submeshIndex = 0;
	EntityHandle entity = InvalidEntity;
};

class RenderSceneProxy : public TickableWorldSubsystem
{
    using BatchFn = std::function<void(const MeshBatch&)>;
public:
    
    virtual void Tick(World* world) override;

	virtual void Shutdown(World* world) override;
    
    void ForEach(BatchFn&& fn) const;

	const Buffer& GetGlobalInstanceBuffer() const
	{
		return mGlobalInstanceBuffer;
	}

	TArrayView<const MeshInstanceData> GetGlobalInstances() const
	{
		return { mGlobalInstances.data(), mTotalInstances };
	}

	TArrayView<const MeshInstance> GetGlobalMeshes() const
	{
		return { mGlobalMeshes.data(), mTotalInstances };
	}

	uint32_t GetBatchCount() const
	{
		return mNumBatches;
	}

private:

	uint32_t mNumBatches = 0;
	uint32_t mTotalInstances = 0;
	Buffer mGlobalInstanceBuffer = {};
    HashMap<AssetReference, MeshBatch> mMeshBatches;

	static constexpr uint32_t MaxMeshInstances = MAX_MESH_INSTANCES;
	static_assert(MaxMeshInstances <= VISIBILITY_INSTANCE_MASK, "MaxMeshInstances exceeds the visibility buffer instance ID bit budget.");

	TArray<MeshInstance, MaxMeshInstances> mGlobalMeshes = {};
	TArray<MeshInstanceData, MaxMeshInstances> mGlobalInstances = {};

};

} // namespace Gleam
