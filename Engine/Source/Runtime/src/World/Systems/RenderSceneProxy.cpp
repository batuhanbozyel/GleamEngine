#include "gpch.h"
#include "RenderSceneProxy.h"

#include "World/World.h"
#include "Renderer/Mesh.h"
#include "Renderer/Material/Material.h"
#include "Renderer/Material/MaterialInstance.h"

using namespace Gleam;

void RenderSceneProxy::OnUpdate(EntityManager& entityManager)
{
    // update static batches
    mStaticBatches.clear();
    entityManager.ForEach<Entity, MeshRenderer>([this](const Entity& entity, const MeshRenderer& meshRenderer)
    {
        GLEAM_ASSERT(meshRenderer.GetMesh().GetSubmeshCount() > 0);
		GLEAM_ASSERT(meshRenderer.GetMaterials().size() == meshRenderer.GetMesh().GetSubmeshCount());

		const auto& materials = meshRenderer.GetMaterials();
		const auto& submeshes = meshRenderer.GetMesh().GetSubmeshDescriptors();
		for (uint32_t i = 0; i < meshRenderer.GetMesh().GetSubmeshCount(); ++i)
		{
			MeshBatch batch = {
				.mesh = meshRenderer.GetMesh(),
				.transform = entity.GetWorldTransform(),
				.submesh = submeshes[i],
				.material = materials[i]
			};

			const auto& baseMaterial = static_cast<const Material*>(batch.material.GetBaseMaterial());
			mStaticBatches[baseMaterial].emplace_back(batch);
		}
    });
    
    // update active camera
    mActiveCamera = nullptr;
    entityManager.ForEach<Entity, Camera>([&](const Entity& entity, const Camera& component)
    {
        if (entity.IsActive())
        {
            mActiveCamera = &entity;
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

const Entity* RenderSceneProxy::GetActiveCamera() const
{
    return mActiveCamera;
}
