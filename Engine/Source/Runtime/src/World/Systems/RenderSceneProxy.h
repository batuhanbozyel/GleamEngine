#pragma once
#include "World/ComponentSystem.h"

namespace Gleam {

class Mesh;
class Entity;
class Material;
class MaterialInstance;

struct MeshBatch
{
	Mesh mesh;
    Float4x4 transform;
	SubmeshDescriptor submesh;
    MaterialInstance material;
};

class RenderSceneProxy : public ComponentSystem
{
    using BatchFn = std::function<void(const Material*, const TArray<MeshBatch>&)>;
public:
    
    virtual void OnUpdate(EntityManager& entityManager) override;
    
    void ForEach(BatchFn&& fn) const;
    
    const Entity* GetActiveCamera() const;

private:
    
    const Entity* mActiveCamera = nullptr;
    
    HashMap<const Material*, TArray<MeshBatch>> mStaticBatches;
    
};

} // namespace Gleam
