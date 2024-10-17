#include "gpch.h"
#include "RenderSceneProxy.h"

#include "Core/Globals.h"
#include "Core/Application.h"

#include "World/World.h"
#include "Assets/AssetManager.h"
#include "Renderer/Material/MaterialSystem.h"

using namespace Gleam;

void RenderSceneProxy::OnUpdate(EntityManager& entityManager)
{
	auto assetManager = Globals::GameInstance->GetSubsystem<AssetManager>();
	
    // update static batches
    mStaticBatches.clear();
	entityManager.ForEach<Entity, MeshRenderer>([&](const Entity& entity, const MeshRenderer& meshRenderer)
	{
		const auto mesh = assetManager->Load<Mesh>(meshRenderer.mesh);
		const auto& submeshes = mesh->GetSubmeshes();

		GLEAM_ASSERT(meshRenderer.materials.size() == submeshes.size());
		for (uint32_t i = 0; i < submeshes.size(); ++i)
		{
			MeshBatch batch = {
				.mesh = mesh,
				.transform = entity.GetWorldTransform(),
				.submesh = submeshes[i],
				.material = assetManager->Load<MaterialInstance>(meshRenderer.materials[i])
			};
			mStaticBatches[batch.material->GetBaseMaterial()].emplace_back(batch);
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
	auto materialSystem = Globals::GameInstance->GetSubsystem<MaterialSystem>();
    for (const auto& [materialRef, batch] : mStaticBatches)
    {
		const auto& material = materialSystem->GetMaterial(materialRef);
        fn(material, batch);
    }
}

const Entity* RenderSceneProxy::GetActiveCamera() const
{
    return mActiveCamera;
}
