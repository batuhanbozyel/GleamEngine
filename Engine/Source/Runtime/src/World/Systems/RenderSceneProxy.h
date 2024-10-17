#pragma once
#include "World/ComponentSystem.h"
#include "Renderer/Mesh.h"
#include "Renderer/Material/MaterialInstance.h"

namespace Gleam {

class Entity;
class Material;

struct MeshBatch
{
	const Mesh* mesh = nullptr;
	const MaterialInstance* material = nullptr;
    Float4x4 transform;
	SubmeshDescriptor submesh;
};

class RenderSceneProxy : public ComponentSystem
{
    using BatchFn = std::function<void(const Material&, const TArray<MeshBatch>&)>;
public:
    
    virtual void OnUpdate(EntityManager& entityManager) override;
    
    void ForEach(BatchFn&& fn) const;
    
    const Entity* GetActiveCamera() const;

private:
    
    const Entity* mActiveCamera = nullptr;

    HashMap<AssetReference, TArray<MeshBatch>> mStaticBatches;
    
};

} // namespace Gleam
