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
#include "Renderer/Renderers/WorldRenderer.h"

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

				auto materialDescriptor = assetManager->LoadDescriptor<MaterialDescriptor>(material);

				auto worldRenderer = renderSystem->GetRenderPipeline(RenderPath::Default)->GetRenderer<WorldRenderer>();
				worldRenderer->RegisterShadingPipeline(batch.material);

				auto rayTracingScene = renderSystem->GetRayTracingScene();
				rayTracingScene->RegisterShadingPipeline(batch.material);

				auto pathTracer = renderSystem->GetRenderPipeline(RenderPath::PathTracing)->GetRenderer<PathTracer>();
				pathTracer->RegisterShadingPipeline(batch.material);
			}
			++batch.numInstances;
		}
	});

	mTotalInstances = 0;
	for (auto& [_, batch] : mMeshBatches)
	{
		if (batch.numInstances == 0)
		{
			continue;
		}
		batch.instanceOffset = mTotalInstances;
		mTotalInstances += batch.numInstances;
		batch.numInstances = 0; // reset to use as write counter in pass 2
	}

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
			uint32_t globalIndex = batch.instanceOffset + batch.numInstances++;

			mGlobalMeshes[globalIndex] = mesh;
			auto& instance = mGlobalInstances[globalIndex];
			instance.positionBuffer = mesh->GetPositionBuffer().GetResourceView();
			instance.interleavedBuffer = mesh->GetInterleavedBuffer().GetResourceView();
			instance.indexBuffer = mesh->GetIndexBuffer().GetResourceView();
			instance.materialBuffer = batch.material->GetBuffer().GetResourceView();
			instance.materialID = materialInstance->GetID();
			instance.transform = entity.GetWorldTransform();
			instance.baseVertex = submesh.baseVertex;
			instance.indexCount = submesh.indexCount;
			instance.firstIndex = submesh.firstIndex;
		}
	});
}

void RenderSceneProxy::Shutdown(World* world)
{
	static auto renderSystem = Globals::Engine->GetSubsystem<RenderSystem>();
	static auto assetManager = Globals::GameInstance->GetSubsystem<AssetManager>();

	world->GetEntityManager().ForEach<Entity, MeshRenderer>([&](const Entity& entity, const MeshRenderer& meshRenderer)
	{
		assetManager->Release(meshRenderer.mesh);
		for (const auto& material : meshRenderer.materials)
		{
			assetManager->Release(material);
		}
	});

	auto device = renderSystem->GetDevice();
	if (mGlobalInstanceBuffer.IsValid())
	{
		device->Dispose(renderSystem->GetAllocator(), mGlobalInstanceBuffer);
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
