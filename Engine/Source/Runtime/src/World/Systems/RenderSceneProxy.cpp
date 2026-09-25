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
#include "Renderer/GraphicsDevice.h"
#include "Renderer/RenderPipeline.h"
#include "Renderer/RayTracingScene.h"
#include "Renderer/CopyCommandBuffer.h"

#include "Renderer/Material/Material.h"
#include "Renderer/Material/MaterialInstance.h"

#include "Renderer/Renderers/PathTracer.h"
#include "Renderer/Renderers/DepthPrepass.h"
#include "Renderer/Renderers/WorldRenderer.h"
#include "Renderer/Renderers/SunShadowRenderer.h"

using namespace Gleam;

void RenderSceneProxy::BuildRecord(const EntityManager& entityManager, EntityHandle entityHandle, const MeshRenderer& meshRenderer)
{
	static auto renderSystem = Globals::Engine->GetSubsystem<RenderSystem>();
	static auto assetManager = Globals::GameInstance->GetSubsystem<AssetManager>();

	const auto key = entt::to_integral(entityHandle);
	uint32_t index = 0;
	const auto it = mRecordLookup.find(key);
	if (it == mRecordLookup.end())
	{
		index = static_cast<uint32_t>(mRecords.size());
		mRecordLookup.emplace_hint(mRecordLookup.end(), key, index);

		const auto& entity = entityManager.GetComponent<Entity>(entityHandle);
		mRecords.push_back(MeshEntityRecord{ .entity = entityHandle, .transform = entity.GetWorldTransform() });
	}
	else
	{
		index = it->second;
	}

	auto& record = mRecords[index];
	TArray<AssetReference> previous = eastl::move(record.acquired);
	record.acquired.clear();
	record.instances.clear();

	const auto mesh = assetManager->Load<Mesh>(meshRenderer.mesh);
	if (mesh == nullptr)
	{
		ReleaseAcquired(previous);
		return;
	}
	record.acquired.push_back(meshRenderer.mesh);

	const auto& submeshes = mesh->GetSubmeshes();
	for (uint32_t submeshIndex = 0; submeshIndex < submeshes.size(); ++submeshIndex)
	{
		const auto& submesh = submeshes[submeshIndex];
		if (submesh.materialIndex >= meshRenderer.materials.size())
		{
			continue;
		}

		const auto& materialInstanceRef = meshRenderer.materials[submesh.materialIndex];
		const auto materialInstance = assetManager->Load<MaterialInstance>(materialInstanceRef);
		if (materialInstance == nullptr)
		{
			continue;
		}
		record.acquired.push_back(materialInstanceRef);

		const auto& materialRef = materialInstance->GetBaseMaterial();
		auto batchIt = mMeshBatches.find(materialRef);
		if (batchIt == mMeshBatches.end())
		{
			const auto material = assetManager->Load<Material>(materialRef);
			if (material == nullptr)
			{
				continue;
			}

			batchIt = mMeshBatches.emplace(materialRef, MeshBatch{ .material = material }).first;
			renderSystem->RegisterShadingPipelines(material);
		}

		MeshInstanceRecord instanceRecord;
		instanceRecord.material = materialRef;
		instanceRecord.mesh = mesh;
		instanceRecord.materialInstance = materialInstance;
		instanceRecord.submeshIndex = submeshIndex;
		record.instances.push_back(instanceRecord);
	}

	ReleaseAcquired(previous);
}

void RenderSceneProxy::ReleaseAcquired(TArray<AssetReference>& acquired)
{
	static auto assetManager = Globals::GameInstance->GetSubsystem<AssetManager>();
	for (const auto& ref : acquired)
	{
		assetManager->Release(ref);
	}
	acquired.clear();
}

void RenderSceneProxy::RemoveRecord(EntityHandle entity)
{
	const auto key = entt::to_integral(entity);
	const auto it = mRecordLookup.find(key);
	if (it != mRecordLookup.end())
	{
		const uint32_t index = it->second;
		ReleaseAcquired(mRecords[index].acquired);

		const uint32_t last = static_cast<uint32_t>(mRecords.size()) - 1;
		if (index != last)
		{
			mRecords[index] = eastl::move(mRecords[last]);
			mRecordLookup[entt::to_integral(mRecords[index].entity)] = index;
		}
		mRecords.pop_back();
		mRecordLookup.erase(key);
	}
}

void RenderSceneProxy::Update(const World* world)
{
	static auto renderSystem = Globals::Engine->GetSubsystem<RenderSystem>();

	if (not mGlobalInstanceBuffer.IsValid())
	{
		BufferDescriptor bufferDesc;
		bufferDesc.name = "GlobalMeshInstanceBuffer";
		bufferDesc.size = sizeof(MeshInstanceData) * MaxMeshInstances;
		mGlobalInstanceBuffer = renderSystem->GetDevice()->CreateBuffer(renderSystem->GetAllocator(), bufferDesc);
	}

	const auto& entityManager = world->GetEntityManager();
	const Tick since = mChangeCursor.Begin(entityManager.GetChangeTracker());
	entityManager.ForEachRemoved<MeshRenderer>(since, [this](EntityHandle entity)
	{
		RemoveRecord(entity);
	});
	entityManager.ForEachChanged<MeshRenderer>(since, [this, &entityManager](EntityHandle entity, const MeshRenderer& meshRenderer)
	{
		BuildRecord(entityManager, entity, meshRenderer);
	});
	mChangeCursor.Commit();

	for (auto& [_, batch] : mMeshBatches)
	{
		batch.numInstances = 0;
	}

	for (const auto& record : mRecords)
	{
		for (const auto& instanceRecord : record.instances)
		{
			++mMeshBatches[instanceRecord.material].numInstances;
		}
	}

	mNumBatches = 0;
	mTotalInstances = 0;
	for (auto& [_, batch] : mMeshBatches)
	{
		if (batch.numInstances == 0)
		{
			continue;
		}
		batch.batchIndex = mNumBatches++;
		batch.instanceOffset = mTotalInstances;
		mTotalInstances += batch.numInstances;
		batch.numInstances = 0; // reset to use as write counter in the fill pass
	}
	GLEAM_ASSERT(mTotalInstances <= MaxMeshInstances, "Instance count exceeds the maximum allowed.");
	GLEAM_ASSERT(mNumBatches <= VISIBILITY_MAX_BATCHES, "Batch count exceeds the visibility buffer batch index bit budget.");

	for (auto& record : mRecords)
	{
		const auto& entity = entityManager.GetComponent<Entity>(record.entity);

		Float4x4 previousTransform = record.transform;
		record.transform = entity.GetWorldTransform();

		for (const auto& instanceRecord : record.instances)
		{
			auto& batch = mMeshBatches[instanceRecord.material];
			const uint32_t globalIndex = batch.instanceOffset + batch.numInstances++;

			const auto mesh = instanceRecord.mesh;
			const auto& submesh = mesh->GetSubmeshes()[instanceRecord.submeshIndex];

			mGlobalMeshes[globalIndex].mesh = mesh;
			mGlobalMeshes[globalIndex].submeshIndex = instanceRecord.submeshIndex;
			mGlobalMeshes[globalIndex].entity = record.entity;

			auto& instance = mGlobalInstances[globalIndex];
			instance.meshBuffer = mesh->GetBuffer().GetResourceView();
			instance.materialBuffer = batch.material->GetBuffer().GetResourceView();
			instance.positionsOffset = static_cast<uint32_t>(mesh->GetPositions().offset);
			instance.interleavedOffset = static_cast<uint32_t>(mesh->GetInterleavedVertices().offset);
			instance.indexOffset = static_cast<uint32_t>(mesh->GetIndices().offset);
			instance.meshletsOffset = static_cast<uint32_t>(mesh->GetMeshlets().offset);
			instance.meshletVertexOffset = static_cast<uint32_t>(mesh->GetMeshletVertices().offset);
			instance.meshletTriangleOffset = static_cast<uint32_t>(mesh->GetMeshletTriangleIndices().offset);
			instance.materialID = instanceRecord.materialInstance->GetID();
			instance.transform = record.transform;
			instance.previousTransform = previousTransform;
			instance.baseVertex = submesh.baseVertex;
			instance.indexCount = submesh.indexCount;
			instance.firstIndex = submesh.firstIndex;
			instance.baseMeshlet = submesh.baseMeshlet;
			instance.meshletCount = submesh.meshletCount;
			instance.cullMode = static_cast<uint32_t>(batch.material->GetDescriptor().cullingMode);
			instance.batchIndex = batch.batchIndex;
		}
	}
}

void RenderSceneProxy::Shutdown(World* world)
{
	static auto assetManager = Globals::GameInstance->GetSubsystem<AssetManager>();
	for (auto& record : mRecords)
	{
		ReleaseAcquired(record.acquired);
	}
	mRecords.clear();
	mRecordLookup.clear();

	for (const auto& [material, batch] : mMeshBatches)
	{
		if (batch.material != nullptr)
		{
			assetManager->Release(material);
		}
	}

	static auto renderSystem = Globals::Engine->GetSubsystem<RenderSystem>();
	auto device = renderSystem->GetDevice();
	if (mGlobalInstanceBuffer.IsValid())
	{
		device->Dispose(renderSystem->GetAllocator(), mGlobalInstanceBuffer, BarrierStage::None);
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
