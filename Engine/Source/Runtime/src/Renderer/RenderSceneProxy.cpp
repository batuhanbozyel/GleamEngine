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
        GLEAM_ASSERT(meshRenderer.GetMesh().GetSubmeshCount() > 0);
		GLEAM_ASSERT(meshRenderer.GetMaterials().size() == meshRenderer.GetMesh().GetSubmeshCount());

		const auto& materials = meshRenderer.GetMaterials();
		const auto& submeshes = meshRenderer.GetMesh().GetSubmeshDescriptors();
		for (uint32_t i = 0; i < meshRenderer.GetMesh().GetSubmeshCount(); ++i)
		{
			MeshBatch batch = {
				.mesh = meshRenderer.GetMesh(),
				.transform = transform.GetWorldTransform(),
				.submesh = submeshes[i],
				.material = materials[i]
			};

			const auto& baseMaterial = static_cast<const Material*>(batch.material.GetBaseMaterial());
			mStaticBatches[baseMaterial].emplace_back(batch);
		}
    });
    
    // update active camera
    mActiveCamera = nullptr;
    world->GetEntityManager().ForEach<Entity, Camera>([&](const Entity& entity, const Camera& component)
    {
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
        fn(material, batch);
    }
}

const Camera* RenderSceneProxy::GetActiveCamera() const
{
    return mActiveCamera;
}
