#include "gpch.h"
#include "RenderSceneProxy.h"

#include "Mesh.h"
#include "World/World.h"
#include "Material/Material.h"
#include "Material/MaterialInstance.h"

using namespace Gleam;

void RenderSceneProxy::Update(const World* world)
{
    // update static batches
    mStaticBatches.clear();
    world->GetEntityManager().ForEach<MeshRenderer, Transform>([this](const MeshRenderer& meshRenderer, const Transform& transform)
    {
        GLEAM_ASSERT(meshRenderer.GetMesh()->GetSubmeshCount() > 0);
        
        const auto& material = meshRenderer.GetMaterial(0);
        const auto& baseMaterial = std::static_pointer_cast<Material>(material->GetBaseMaterial());
        
        MeshBatch batch = { 
            .mesh = meshRenderer.GetMesh().get(),
            .material = material.get(),
            .transform = transform.GetWorldTransform()
        };
        mStaticBatches[baseMaterial].emplace_back(batch);
    });
    
    // update active camera
    mActiveCamera = nullptr;
    world->GetEntityManager().ForEach<Camera>([&](EntityHandle handle, const Camera& component)
    {
        const auto& entity = world->GetEntityManager().GetComponent<Entity>(handle);
        if (entity.IsActive())
        {
            mActiveCamera = &component;
        }
    });
}

void RenderSceneProxy::ForEach(BatchFn&& fn) const
{
    for (const auto& [material, batch] : mStaticBatches)
    {
        fn(material.get(), batch);
    }
}

const Camera* RenderSceneProxy::GetActiveCamera() const
{
    return mActiveCamera;
}
