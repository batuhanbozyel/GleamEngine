#include "gpch.h"
#include "RenderSceneProxy.h"

#include "Core/Globals.h"
#include "Core/Engine.h"
#include "Core/Application.h"
#include "Assets/AssetManager.h"

#include "World/World.h"
#include "World/EntityManager.h"

#include "Renderer/Mesh.h"
#include "Renderer/RenderSystem.h"
#include "Renderer/CopyCommandBuffer.h"
#include "Renderer/Material/Material.h"
#include "Renderer/Material/MaterialInstance.h"
#include "Renderer/Renderers/WorldRenderer.h"

using namespace Gleam;

void RenderSceneProxy::OnUpdate(EntityManager& entityManager)
{
	static auto renderSystem = Globals::Engine->GetSubsystem<RenderSystem>();
	static auto assetManager = Globals::GameInstance->GetSubsystem<AssetManager>();

	for (auto& [_, batch] : mMeshBatches)
	{
		batch.numInstances = 0;
	}

	// update mesh batches
	entityManager.ForEach<Entity, MeshRenderer>([&](const Entity& entity, const MeshRenderer& meshRenderer)
	{
		const auto mesh = assetManager->Load<Mesh>(meshRenderer.mesh);
		const auto& submeshes = mesh->GetSubmeshes();

		for (const auto& submesh : submeshes)
		{
			const auto materialInstance = assetManager->Load<MaterialInstance>(meshRenderer.materials[submesh.materialIndex]);
			const auto& material = materialInstance->GetBaseMaterial();

			auto& batch = mMeshBatches[material];
			if (batch.instanceBuffer.IsValid() == false)
			{
				auto device = renderSystem->GetDevice();

				HeapDescriptor heapDesc;
				heapDesc.name = "MeshInstanceData";
				heapDesc.memoryType = MemoryType::GPU;
				heapDesc.size = sizeof(MeshInstanceData) * MeshBatch::MaxMeshInstances;

				BufferDescriptor bufferDesc;
				bufferDesc.name = "MeshInstanceBuffer";
				bufferDesc.size = sizeof(MeshInstanceData) * MeshBatch::MaxMeshInstances;

				// TODO: Rework this:
				// we should be using transient allocator here instead of persistent
				batch.instanceBuffer = device->CreateBuffer(renderSystem->GetAllocator(), bufferDesc); 
				batch.material = assetManager->Get<Material>(material);

				auto worldRenderer = renderSystem->GetRenderer<WorldRenderer>();
				auto materialDescriptor = assetManager->LoadDescriptor<MaterialDescriptor>(material);
				worldRenderer->RegisterShadingPipeline(materialDescriptor, batch.material->GetPipelineHash());
			}

			batch.meshes[batch.numInstances] = mesh;
			batch.instances[batch.numInstances].positionBuffer = mesh->GetPositionBuffer().GetResourceView();
			batch.instances[batch.numInstances].interleavedBuffer = mesh->GetInterleavedBuffer().GetResourceView();
			batch.instances[batch.numInstances].indexBuffer = mesh->GetIndexBuffer().GetResourceView();
			batch.instances[batch.numInstances].materialID = materialInstance->GetID();
			batch.instances[batch.numInstances].transform = entity.GetWorldTransform();
			batch.instances[batch.numInstances].baseVertex = submesh.baseVertex;
			batch.instances[batch.numInstances].indexCount = submesh.indexCount;
			batch.instances[batch.numInstances].firstIndex = submesh.firstIndex;
			++batch.numInstances;
		}
	});
}

void RenderSceneProxy::OnDestroy(EntityManager& entityManager)
{
	static auto renderSystem = Globals::Engine->GetSubsystem<RenderSystem>();
	static auto assetManager = Globals::GameInstance->GetSubsystem<AssetManager>();

	entityManager.ForEach<Entity, MeshRenderer>([&](const Entity& entity, const MeshRenderer& meshRenderer)
	{
		assetManager->Release(meshRenderer.mesh);
		for (const auto& material : meshRenderer.materials)
		{
			assetManager->Release(material);
		}
	});

	auto device = renderSystem->GetDevice();
	for (auto& [_, batch] : mMeshBatches)
	{
		device->Dispose(renderSystem->GetAllocator(), batch.instanceBuffer);
	}
	mMeshBatches.clear();
}

void RenderSceneProxy::ForEach(BatchFn&& fn) const
{
    for (const auto& [_, batch] : mMeshBatches)
    {
        fn(batch);
    }
}
