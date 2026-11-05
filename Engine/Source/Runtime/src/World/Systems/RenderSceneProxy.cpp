#include "gpch.h"
#include "RenderSceneProxy.h"

#include "Core/Globals.h"
#include "Core/Engine.h"
#include "Core/Application.h"
#include "Assets/AssetManager.h"

#include "World/World.h"
#include "World/EntityManager.h"
#include "World/Systems/CameraSystem.h"

#include "Renderer/Mesh.h"
#include "Renderer/SceneView.h"
#include "Renderer/RenderSystem.h"
#include "Renderer/GraphicsDevice.h"
#include "Renderer/RayTracingScene.h"
#include "Renderer/CommandBuffer.h"
#include "Renderer/CopyCommandBuffer.h"
#include "Renderer/Allocator/GPUAllocator.h"

#include "Renderer/Material/Material.h"
#include "Renderer/Material/MaterialInstance.h"

#include "Renderer/Renderers/PathTracer.h"
#include "Renderer/Renderers/DepthPrepass.h"
#include "Renderer/Renderers/WorldRenderer.h"
#include "Renderer/Renderers/SunShadowRenderer.h"

using namespace Gleam;

static void ComputeWorldBounds(const Mesh& mesh, const Transform& transform, Float3& center, float& radius)
{
	const auto& bounds = mesh.GetBounds();
	if (bounds.IsValid())
	{
		center = transform.position + transform.rotation * ((bounds.min + bounds.max) * 0.5f * transform.scale);
		radius = Math::Length(bounds.Extent()) * 0.5f * transform.scale;
	}
	else
	{
		center = transform.position;
		radius = Math::Infinity;
	}
}

// Fraction of the view height the sphere covers; projection.m[5] is 1 / tan(fovY / 2) or 2 / orthographic height
static float ComputeScreenCoverage(const Float3& center, float radius, const Camera& camera, const Float4x4& projection, const Float3& cameraPosition)
{
	float coverage = 1.0f;
	if (camera.projectionType == ProjectionType::Perspective)
	{
		const float distance = Math::Length(center - cameraPosition);
		if (distance > radius)
		{
			coverage = radius * projection.m[5] / distance;
		}
	}
	else
	{
		coverage = radius * projection.m[5];
	}
	return coverage;
}

static bool IsSphereInFrustum(const Frustum& frustum, const Float3& center, float radius)
{
	for (const auto& plane : frustum.planes)
	{
		if (Math::Dot(plane.normal, center) + plane.distance < -radius)
		{
			return false;
		}
	}
	return true;
}

static uint32_t SelectMeshLod(Mesh* mesh, float screenCoverage)
{
	static constexpr float kLodScreenCoverage = 0.5f;

	const float maxLod = static_cast<float>(mesh->GetLodCount() - 1);
	const uint32_t lod = static_cast<uint32_t>(Math::Clamp(Math::Ceil(Math::Log2(kLodScreenCoverage / screenCoverage)), 0.0f, maxLod));

	mesh->RequestLod(lod);
	return lod;
}

void SceneViewList::Insert(SceneView* view)
{
	auto& views = view->GetRenderTexture() ? targetViews : swapchainViews;
	const auto it = eastl::upper_bound(views.begin(), views.end(), view, [](const SceneView* lhs, const SceneView* rhs)
	{
		return lhs->GetPriority() < rhs->GetPriority();
	});
	views.insert(it, view);
}

void SceneViewList::Remove(SceneView* view)
{
	targetViews.erase_first(view);
	swapchainViews.erase_first(view);
}

void RenderSceneProxy::Initialize(World* world)
{
	mWorld = world;
	mMeshRendererRemoved = world->GetEntityManager().OnComponentRemoved<MeshRenderer, &RenderSceneProxy::OnMeshRendererRemoved>(*this);
	mCameraRemoved = world->GetEntityManager().OnComponentRemoved<Camera, &RenderSceneProxy::OnCameraRemoved>(*this);
}

void RenderSceneProxy::OnMeshRendererRemoved(EntityHandle entity)
{
	RemoveRecord(entity);
}

void RenderSceneProxy::BuildRecord(const EntityManager& entityManager, EntityHandle entityHandle, const MeshRenderer& meshRenderer)
{
	static auto renderSystem = Globals::Engine->GetSubsystem<RenderSystem>();
	static auto assetManager = Globals::GameInstance->GetSubsystem<AssetManager>();

	uint32_t index = 0;
	uint32_t key = entt::to_integral(entityHandle);

	auto it = mRecordLookup.find(key);
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
	RemoveDraws(index);
	record.instances.clear();

	TArray<AssetReference> previous = eastl::move(record.acquired);
	record.acquired.clear();

	const auto mesh = assetManager->Load<Mesh>(meshRenderer.mesh);
	record.mesh = mesh;
	record.lod = 0;
	if (mesh == nullptr)
	{
		ReleaseAcquired(previous);
		return;
	}
	record.acquired.push_back(meshRenderer.mesh);

	const auto& submeshes = mesh->GetSubmeshes(0);
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
		uint32_t batchSlot = 0;
		auto batchIt = mBatchLookup.find(materialRef);
		if (batchIt != mBatchLookup.end())
		{
			batchSlot = batchIt->second;
		}
		else
		{
			const auto material = assetManager->Load<Material>(materialRef);
			if (material == nullptr)
			{
				continue;
			}

			batchSlot = static_cast<uint32_t>(mBatches.size());
			mBatches.push_back(MeshBatch{ .material = material });
			mBatchLookup.emplace(materialRef, batchSlot);
			renderSystem->RegisterShadingPipelines(material);
		}

		record.instances.push_back(MeshInstanceRecord{ .material = materialInstance, .submeshIndex = submeshIndex, .batch = batchSlot });
	}

	AddDraws(index);
	ReleaseAcquired(previous);
}

void RenderSceneProxy::AddDraws(uint32_t recordIndex)
{
	auto& record = mRecords[recordIndex];
	for (uint32_t i = 0; i < record.instances.size(); ++i)
	{
		auto& instance = record.instances[i];
		auto& draws = mBatches[instance.batch].draws;
		instance.draw = static_cast<uint32_t>(draws.size());
		draws.push_back(MeshDraw{ .record = recordIndex, .instance = i });
	}
}

void RenderSceneProxy::RemoveDraws(uint32_t recordIndex)
{
	for (const auto& instance : mRecords[recordIndex].instances)
	{
		auto& draws = mBatches[instance.batch].draws;
		const MeshDraw moved = draws.back();
		draws[instance.draw] = moved;
		mRecords[moved.record].instances[moved.instance].draw = instance.draw;
		draws.pop_back();
	}
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
	uint32_t key = entt::to_integral(entity);

	auto it = mRecordLookup.find(key);
	if (it != mRecordLookup.end())
	{
		uint32_t index = it->second;
		RemoveDraws(index);
		ReleaseAcquired(mRecords[index].acquired);

		uint32_t last = static_cast<uint32_t>(mRecords.size()) - 1;
		if (index != last)
		{
			mRecords[index] = eastl::move(mRecords[last]);
			for (const auto& instance : mRecords[index].instances)
			{
				mBatches[instance.batch].draws[instance.draw].record = index;
			}
			mRecordLookup[entt::to_integral(mRecords[index].entity)] = index;
		}
		mRecords.pop_back();
		mRecordLookup.erase(key);
	}
}

void RenderSceneProxy::Update(const World* world)
{
	static auto renderSystem = Globals::Engine->GetSubsystem<RenderSystem>();

	const auto& entityManager = world->GetEntityManager();
	const Tick since = mChangeCursor.Begin(entityManager.GetChangeTracker());
	entityManager.ForEachChanged<MeshRenderer>(since, [this, &entityManager](EntityHandle entity, const MeshRenderer& meshRenderer)
	{
		BuildRecord(entityManager, entity, meshRenderer);
	});

	entityManager.ForEachChanged<Camera>(since, [this](EntityHandle entity, const Camera& component)
	{
		auto view = renderSystem->AcquireSceneView(mWorld, entity);
		mViews.Remove(view);
		mViews.Insert(view);
	});
	mChangeCursor.Commit();

	mMainCamera = InvalidEntity;
	const auto cameraSystem = world->GetSystem<CameraSystem>();
	if (cameraSystem->Enabled)
	{
		mMainCamera = cameraSystem->GetMainCamera(entityManager);
	}
	else
	{
		// When the CameraSystem is disabled, the highest-priority camera is treated as the main camera
		const auto it = eastl::find_if(mViews.targetViews.rbegin(), mViews.targetViews.rend(), [&entityManager](const SceneView* view)
		{
			return entityManager.GetComponent<Entity>(view->GetCameraEntity()).IsActive();
		});
		if (it != mViews.targetViews.rend())
		{
			mMainCamera = (*it)->GetCameraEntity();
		}
	}

	if (mMainCamera == InvalidEntity)
	{
		return;
	}

	const auto& cameraComponent = entityManager.GetComponent<Camera>(mMainCamera);
	Float3 cameraPosition = entityManager.GetComponent<Entity>(mMainCamera).GetWorldPosition();

	const auto resolution = renderSystem->GetSceneView(mWorld, mMainCamera)->GetResolution();
	if (resolution.width < 1.0f or resolution.height < 1.0f)
	{
		return;
	}

	const float aspectRatio = resolution.width / resolution.height;
	const auto projection = cameraComponent.GetProjectionMatrix(aspectRatio);

	for (auto& record : mRecords)
	{
		const auto& entity = entityManager.GetComponent<Entity>(record.entity);
		const auto& worldTransform = entity.GetWorldTransform();

		record.prevTransform = record.transform;
		record.transform = worldTransform;

		if (record.mesh)
		{
			ComputeWorldBounds(*record.mesh, worldTransform, record.worldCenter, record.worldRadius);

			float coverage = ComputeScreenCoverage(record.worldCenter, record.worldRadius, cameraComponent, projection, cameraPosition);
			record.lod = SelectMeshLod(record.mesh, coverage);
		}
	}

	mNumBatches = 0;
	mTotalInstances = 0;
	for (auto& batch : mBatches)
	{
		if (not batch.draws.empty())
		{
			batch.batchIndex = mNumBatches++;
			batch.instanceOffset = mTotalInstances;
			mTotalInstances += static_cast<uint32_t>(batch.draws.size());
		}
	}

	GLEAM_ASSERT(mTotalInstances <= MaxMeshInstances, "Instance count exceeds the maximum allowed.");
	GLEAM_ASSERT(mNumBatches <= VISIBILITY_MAX_BATCHES, "Batch count exceeds the visibility buffer batch index bit budget.");
}

void RenderSceneProxy::BuildInstanceBuffer(const CommandBuffer* cmd, GPUAllocator* allocator)
{
	static auto renderSystem = Globals::Engine->GetSubsystem<RenderSystem>();
	auto device = renderSystem->GetDevice();

	size_t instanceBufferSize = sizeof(MeshInstanceData) * Math::Max(mTotalInstances, 1u);
	auto instanceStagingBuffer = device->CreateBuffer(allocator, BufferDescriptor{ .name = "MeshInstanceStagingBuffer", .memoryType = MemoryType::CPU, .size = instanceBufferSize });
	mInstanceBuffer = device->CreateBuffer(allocator, BufferDescriptor{ .name = "MeshInstanceBuffer", .memoryType = MemoryType::GPU, .size = instanceBufferSize });

	auto instances = static_cast<MeshInstanceData*>(instanceStagingBuffer.GetContents());
	for (const auto& batch : mBatches)
	{
		ForEachDraw(batch, [&](uint32_t instanceID, const MeshEntityRecord& record, const MeshInstanceRecord& instanceRecord)
		{
			const auto& submesh = record.mesh->GetSubmesh(record.lod, instanceRecord.submeshIndex);

			MeshInstanceData& instance = instances[instanceID];
			instance.meshBuffer = record.mesh->GetBuffer(record.lod).GetResourceView();
			instance.materialBuffer = batch.material->GetBuffer().GetResourceView();
			instance.positionsOffset = static_cast<uint32_t>(record.mesh->GetPositions(record.lod).offset);
			instance.interleavedOffset = static_cast<uint32_t>(record.mesh->GetInterleavedVertices(record.lod).offset);
			instance.indexOffset = static_cast<uint32_t>(record.mesh->GetIndices(record.lod).offset);
			instance.meshletsOffset = static_cast<uint32_t>(record.mesh->GetMeshlets(record.lod).offset);
			instance.meshletVertexOffset = static_cast<uint32_t>(record.mesh->GetMeshletVertices(record.lod).offset);
			instance.meshletTriangleOffset = static_cast<uint32_t>(record.mesh->GetMeshletTriangleIndices(record.lod).offset);
			instance.materialID = instanceRecord.material->GetID();
			instance.transform = record.transform;
			instance.previousTransform = record.prevTransform;
			instance.baseVertex = submesh.baseVertex;
			instance.indexCount = submesh.indexCount;
			instance.firstIndex = submesh.firstIndex;
			instance.baseMeshlet = submesh.baseMeshlet;
			instance.meshletCount = submesh.meshletCount;
			instance.cullMode = static_cast<uint32_t>(batch.material->GetDescriptor().cullingMode);
			instance.batchIndex = batch.batchIndex;
		});
	}

	const auto& allocation = allocator->GetAllocation(mInstanceBuffer.GetHandle());
	cmd->BeginComputePass("MeshInstanceUpload");

	BarrierGroup copyBarrier;
	copyBarrier.bufferBarriers.push_back({
		.resource = mInstanceBuffer.GetHandle(),
		.srcStage = allocation.aliasStage,
		.dstStage = BarrierStage::Copy,
		.srcAccess = BarrierAccess::None,
		.dstAccess = BarrierAccess::CopyDest
		});
	cmd->Barrier(copyBarrier);

	cmd->CopyBuffer(instanceStagingBuffer, mInstanceBuffer);

	BarrierGroup readBarrier;
	readBarrier.bufferBarriers.push_back({
		.resource = mInstanceBuffer.GetHandle(),
		.srcStage = BarrierStage::Copy,
		.dstStage = BarrierStage::AllShading,
		.srcAccess = BarrierAccess::CopyDest,
		.dstAccess = BarrierAccess::ShaderResource
		});
	cmd->Barrier(readBarrier);
	cmd->EndComputePass();

	device->Dispose(allocator, instanceStagingBuffer, BarrierStage::None);
}

void RenderSceneProxy::ReleaseInstanceBuffer(GPUAllocator* allocator)
{
	static auto renderSystem = Globals::Engine->GetSubsystem<RenderSystem>();
	renderSystem->GetDevice()->Dispose(allocator, mInstanceBuffer, BarrierStage::AllShading);
}

void RenderSceneProxy::Cull(const Frustum& frustum, MeshDrawList& drawList) const
{
	drawList.draws.clear();
	drawList.batches.resize(mNumBatches);
	for (const auto& batch : mBatches)
	{
		if (not batch.draws.empty())
		{
			auto& range = drawList.batches[batch.batchIndex];
			range.first = static_cast<uint32_t>(drawList.draws.size());
			for (uint32_t d = 0; d < batch.draws.size(); ++d)
			{
				const auto& record = mRecords[batch.draws[d].record];
				if (IsSphereInFrustum(frustum, record.worldCenter, record.worldRadius))
				{
					drawList.draws.push_back(d);
				}
			}
			range.count = static_cast<uint32_t>(drawList.draws.size()) - range.first;
		}
	}
}

void RenderSceneProxy::Shutdown(World* world)
{
	mMeshRendererRemoved.Reset();

	static auto assetManager = Globals::GameInstance->GetSubsystem<AssetManager>();
	for (auto& record : mRecords)
	{
		ReleaseAcquired(record.acquired);
	}
	mRecords.clear();
	mRecordLookup.clear();

	for (const auto& [materialRef, _] : mBatchLookup)
	{
		assetManager->Release(materialRef);
	}
	mBatches.clear();
	mBatchLookup.clear();

	mCameraRemoved.Reset();
	mViews = {};
	mMainCamera = InvalidEntity;
	static auto renderSystem = Globals::Engine->GetSubsystem<RenderSystem>();
	renderSystem->ReleaseSceneViews(world);
}

void RenderSceneProxy::OnCameraRemoved(EntityHandle entity)
{
	static auto renderSystem = Globals::Engine->GetSubsystem<RenderSystem>();
	if (auto view = renderSystem->GetSceneView(mWorld, entity))
	{
		mViews.Remove(view);
	}
	renderSystem->ReleaseSceneView(mWorld, entity);
}

void RenderSceneProxy::ForEach(BatchFn&& fn) const
{
    for (const auto& batch : mBatches)
    {
        if (not batch.draws.empty())
        {
            fn(batch);
        }
    }
}
