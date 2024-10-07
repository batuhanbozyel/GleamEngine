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
    // update static batches
    mStaticBatches.clear();
	entityManager.ForEach<Entity, MeshRenderer>([&](const Entity& entity, const MeshRenderer& meshRenderer)
	{
		const auto& mesh = GetMesh(meshRenderer.mesh);
		const auto& submeshes = mesh.GetSubmeshes();

		GLEAM_ASSERT(meshRenderer.materials.size() == submeshes.size());
		for (uint32_t i = 0; i < submeshes.size(); ++i)
		{
			MeshBatch batch = {
				.mesh = mesh,
				.transform = entity.GetWorldTransform(),
				.submesh = submeshes[i],
				.material = GetMaterialInstance(meshRenderer.materials[i])
			};
			mStaticBatches[batch.material.GetBaseMaterial()].emplace_back(batch);
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

void RenderSceneProxy::OnDestroy(EntityManager& entityManager)
{
	for (auto& [_, mesh] : mMeshes)
	{
		mesh.Dispose();
	}
	mMeshes.clear();
	mMaterialInstances.clear();
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

const Mesh& RenderSceneProxy::GetMesh(const AssetReference& ref)
{
	auto it = mMeshes.find(ref);
	if (it != mMeshes.end())
	{
		return it->second;
	}

	auto assetManager = Globals::GameInstance->GetSubsystem<AssetManager>();
	auto descriptor = assetManager->Get<MeshDescriptor>(ref);
	return mMeshes.emplace_hint(mMeshes.end(),
								std::piecewise_construct,
								std::forward_as_tuple(ref),
								std::forward_as_tuple(descriptor))->second;
}

const MaterialInstance& RenderSceneProxy::GetMaterialInstance(const AssetReference& ref)
{
	auto it = mMaterialInstances.find(ref);
	if (it != mMaterialInstances.end())
	{
		return it->second;
	}

	auto assetManager = Globals::GameInstance->GetSubsystem<AssetManager>();
	auto descriptor = assetManager->Get<MaterialInstanceDescriptor>(ref);
	return mMaterialInstances.emplace_hint(mMaterialInstances.end(),
										   std::piecewise_construct,
										   std::forward_as_tuple(ref),
										   std::forward_as_tuple(descriptor))->second;
}

const Entity* RenderSceneProxy::GetActiveCamera() const
{
    return mActiveCamera;
}
