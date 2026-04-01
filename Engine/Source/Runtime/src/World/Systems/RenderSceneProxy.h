#pragma once
#include "World/WorldSubsystem.h"
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

	TArrayView<Mesh* const> GetGlobalMeshes() const
	{
		return { mGlobalMeshes.data(), mTotalInstances };
	}

private:

	uint32_t mTotalInstances = 0;
	Buffer mGlobalInstanceBuffer = {};
    HashMap<AssetReference, MeshBatch> mMeshBatches;

	static constexpr uint32_t MaxMeshInstances = 65536;
	TArray<Mesh*, MaxMeshInstances> mGlobalMeshes = {};
	TArray<MeshInstanceData, MaxMeshInstances> mGlobalInstances = {};

};

} // namespace Gleam
