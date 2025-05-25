#pragma once
#include "World/ComponentSystem.h"
#include "Renderer/MeshDescriptor.h"
#include "Assets/AssetReference.h"
#include "Container/Hash.h"
#include "Math/Float4x4.h"

#include <functional>

namespace Gleam {

class Mesh;
class Entity;
class Material;
class MaterialInstance;

struct MeshBatch
{
	const Mesh* mesh = nullptr;
	const MaterialInstance* material = nullptr;
    Float4x4 transform;
	SubmeshDescriptor submesh;
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

    HashMap<AssetReference, TArray<MeshBatch>> mStaticBatches;
    
};

} // namespace Gleam
