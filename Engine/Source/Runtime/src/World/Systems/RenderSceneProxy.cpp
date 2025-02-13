#include "gpch.h"
#include "RenderSceneProxy.h"

#include "Core/Globals.h"
#include "Core/Engine.h"
#include "Core/Application.h"

#include "World/World.h"
#include "Assets/AssetManager.h"
#include "Renderer/RenderSystem.h"

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
			const auto material = assetManager->Load<MaterialInstance>(meshRenderer.materials[i]);
			MeshBatch batch = {
				.mesh = mesh,
				.material = material,
				.transform = entity.GetWorldTransform(),
				.submesh = submeshes[i]
			};
			mStaticBatches[batch.material->GetBaseMaterial()].emplace_back(batch);
		}
	});
	auto renderSystem = Globals::Engine->GetSubsystem<RenderSystem>();
	renderSystem->GetUploadManager()->Execute();
	renderSystem->GetUploadManager()->WaitUntilCompleted();
    
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
	auto assetManager = Globals::GameInstance->GetSubsystem<AssetManager>();
    for (const auto& [materialRef, batch] : mStaticBatches)
    {
		auto material = assetManager->Get<Material>(materialRef);
        fn(material, batch);
    }
}

const Entity* RenderSceneProxy::GetActiveCamera() const
{
    return mActiveCamera;
}
