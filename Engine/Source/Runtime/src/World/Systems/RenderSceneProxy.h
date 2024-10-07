#pragma once
#include "World/ComponentSystem.h"
#include "Renderer/Mesh.h"
#include "Renderer/Material/MaterialInstance.h"

namespace Gleam {

class Entity;
class Material;

struct MeshBatch
{
	Mesh mesh;
    Float4x4 transform;
	SubmeshDescriptor submesh;
    MaterialInstance material;
};

class RenderSceneProxy : public ComponentSystem
{
    using BatchFn = std::function<void(const Material&, const TArray<MeshBatch>&)>;
public:
    
    virtual void OnUpdate(EntityManager& entityManager) override;

	virtual void OnDestroy(EntityManager& entityManager) override;
    
    void ForEach(BatchFn&& fn) const;
    
    const Entity* GetActiveCamera() const;

private:

	const Mesh& GetMesh(const AssetReference& ref);

	const MaterialInstance& GetMaterialInstance(const AssetReference& ref);
    
    const Entity* mActiveCamera = nullptr;

	HashMap<AssetReference, Mesh> mMeshes;

	HashMap<AssetReference, MaterialInstance> mMaterialInstances;

    HashMap<AssetReference, TArray<MeshBatch>> mStaticBatches;
    
};

} // namespace Gleam
