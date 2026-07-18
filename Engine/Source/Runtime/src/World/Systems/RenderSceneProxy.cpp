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

void RenderSceneProxy::Tick(World* world)
{
	static auto renderSystem = Globals::Engine->GetSubsystem<RenderSystem>();
	static auto assetManager = Globals::GameInstance->GetSubsystem<AssetManager>();

	if (not mGlobalInstanceBuffer.IsValid())
	{
		BufferDescriptor bufferDesc;
		bufferDesc.name = "GlobalMeshInstanceBuffer";
		bufferDesc.size = sizeof(MeshInstanceData) * MaxMeshInstances;
		mGlobalInstanceBuffer = renderSystem->GetDevice()->CreateBuffer(renderSystem->GetAllocator(), bufferDesc);
	}

	for (auto& [_, batch] : mMeshBatches)
	{
		batch.numInstances = 0;
	}

	// Setup batches
	world->GetEntityManager().ForEach<Entity, MeshRenderer>([&](const Entity& entity, const MeshRenderer& meshRenderer)
	{
		const auto mesh = assetManager->Has<Mesh>(meshRenderer.mesh) ? assetManager->Get<Mesh>(meshRenderer.mesh): assetManager->Load<Mesh>(meshRenderer.mesh);
		const auto& submeshes = mesh->GetSubmeshes();

		for (const auto& submesh : submeshes)
		{
			const auto materialInstance = assetManager->Has<MaterialInstance>(meshRenderer.materials[submesh.materialIndex]) ?
				assetManager->Get<MaterialInstance>(meshRenderer.materials[submesh.materialIndex]) :
				assetManager->Load<MaterialInstance>(meshRenderer.materials[submesh.materialIndex]);
			const auto& material = materialInstance->GetBaseMaterial();

			auto& batch = mMeshBatches[material];
			if (batch.material == nullptr)
			{
				batch.material = assetManager->Get<Material>(material);
				renderSystem->RegisterShadingPipelines(batch.material);
			}
			++batch.numInstances;
		}
	});

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
		batch.numInstances = 0; // reset to use as write counter in pass 2
	}
	GLEAM_ASSERT(mTotalInstances <= MaxMeshInstances, "Instance count exceeds the maximum allowed.");
	GLEAM_ASSERT(mNumBatches <= VISIBILITY_MAX_BATCHES, "Batch count exceeds the visibility buffer batch index bit budget.");

	world->GetEntityManager().ForEach<Entity, MeshRenderer>([&](const Entity& entity, const MeshRenderer& meshRenderer)
	{
		const auto mesh = assetManager->Has<Mesh>(meshRenderer.mesh) ? assetManager->Get<Mesh>(meshRenderer.mesh): assetManager->Load<Mesh>(meshRenderer.mesh);
		const auto& submeshes = mesh->GetSubmeshes();

		for (uint32_t submeshIndex = 0; submeshIndex < submeshes.size(); ++submeshIndex)
		{
			const auto& submesh = submeshes[submeshIndex];
			const auto materialInstance = assetManager->Has<MaterialInstance>(meshRenderer.materials[submesh.materialIndex]) ?
				assetManager->Get<MaterialInstance>(meshRenderer.materials[submesh.materialIndex]) :
				assetManager->Load<MaterialInstance>(meshRenderer.materials[submesh.materialIndex]);
			const auto& material = materialInstance->GetBaseMaterial();

			auto& batch = mMeshBatches[material];
			uint32_t globalIndex = batch.instanceOffset + batch.numInstances++;

			mGlobalMeshes[globalIndex].mesh = mesh;
			mGlobalMeshes[globalIndex].submeshIndex = submeshIndex;

			auto& instance = mGlobalInstances[globalIndex];
			instance.meshBuffer = mesh->GetBuffer().GetResourceView();
			instance.materialBuffer = batch.material->GetBuffer().GetResourceView();
			instance.positionsOffset = static_cast<uint32_t>(mesh->GetPositions().offset);
			instance.interleavedOffset = static_cast<uint32_t>(mesh->GetInterleavedVertices().offset);
			instance.indexOffset = static_cast<uint32_t>(mesh->GetIndices().offset);
			instance.meshletsOffset = static_cast<uint32_t>(mesh->GetMeshlets().offset);
			instance.meshletVertexOffset = static_cast<uint32_t>(mesh->GetMeshletVertices().offset);
			instance.meshletTriangleOffset = static_cast<uint32_t>(mesh->GetMeshletTriangleIndices().offset);
			instance.materialID = materialInstance->GetID();
			instance.transform = entity.GetWorldTransform();
			instance.previousTransform = instance.transform; // TODO: store previous transform
			instance.baseVertex = submesh.baseVertex;
			instance.indexCount = submesh.indexCount;
			instance.firstIndex = submesh.firstIndex;
			instance.baseMeshlet = submesh.baseMeshlet;
			instance.meshletCount = submesh.meshletCount;
			instance.cullMode = static_cast<uint32_t>(batch.material->GetDescriptor().cullingMode);
			instance.batchIndex = batch.batchIndex;
		}
	});
}

void RenderSceneProxy::Shutdown(World* world)
{
	world->GetEntityManager().ForEach<Entity, MeshRenderer>([&](const Entity& entity, const MeshRenderer& meshRenderer)
	{
		static auto assetManager = Globals::GameInstance->GetSubsystem<AssetManager>();
		assetManager->Release(meshRenderer.mesh);

		for (const auto& material : meshRenderer.materials)
		{
			assetManager->Release(material);
		}
	});

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
