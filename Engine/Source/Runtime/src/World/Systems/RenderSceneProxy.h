#pragma once
#include "World/ComponentSystem.h"
#include "Renderer/Heap.h"
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
	static constexpr uint32_t MaxMeshInstances = 65536;

	Material* material = nullptr;
	Heap instanceHeap = {};
	Buffer instanceBuffer = {};
	uint32_t numInstances = 0;

	TArray<Mesh*, MaxMeshInstances> meshes;
	TArray<MeshInstanceData, MaxMeshInstances> instances = {};
};

class RenderSceneProxy : public ComponentSystem
{
    using BatchFn = std::function<void(const MeshBatch&)>;
public:
    
    virtual void OnUpdate(EntityManager& entityManager) override;
    
    void ForEach(BatchFn&& fn) const;
    
    const Entity* GetActiveCamera() const;

private:
    
    const Entity* mActiveCamera = nullptr;

    HashMap<AssetReference, MeshBatch> mMeshBatches;
    
};

} // namespace Gleam
