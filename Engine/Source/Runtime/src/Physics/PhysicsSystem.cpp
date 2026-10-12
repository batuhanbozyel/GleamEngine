#include "gpch.h"
#include "PhysicsSystem.h"
#include "Collider.h"
#include "World/Components/RigidBody.h"
#include "World/EntityManager.h"
#include "Assets/AssetManager.h"
#include "Renderer/Mesh.h"

using namespace Gleam;

void PhysicsSystem::OnCreate(EntityManager& entityManager)
{
	mPhysicsWorld = CreateScope<PhysicsWorld>(mGravity);
	mRigidBodyRemoved = entityManager.OnComponentRemoved<RigidBody, &PhysicsSystem::OnRigidBodyRemoved>(*this);
}

void PhysicsSystem::OnRigidBodyRemoved(EntityHandle entity)
{
	const uint32_t index = FindRigidBodyIndex(entity);
	if (index != InvalidRecordIndex)
	{
		DestroyRigidBodyRecord(mRigidBodies[index]);

		const uint32_t last = static_cast<uint32_t>(mRigidBodies.size()) - 1;
		if (index != last)
		{
			mRigidBodies[index] = eastl::move(mRigidBodies[last]);
			mRigidBodyIndices[entt::to_entity(mRigidBodies[index].entity)] = index;
		}
		mRigidBodies.pop_back();
		mRigidBodyIndices[entt::to_entity(entity)] = InvalidRecordIndex;
	}
}

void PhysicsSystem::OnDestroy(EntityManager& entityManager)
{
	mRigidBodyRemoved.Reset();
	mRigidBodies.clear();
	mRigidBodyIndices.clear();
	mPhysicsWorld.reset();
	mConvexHulls.clear();
	mTriangleMeshes.clear();
}

void PhysicsSystem::OnFixedUpdate(EntityManager& entityManager)
{
	const auto& tracker = entityManager.GetChangeTracker();
	if (tracker.GetTick() != mChangeCursor.GetLastSeen())
	{
		const Tick since = mChangeCursor.Begin(tracker);
		entityManager.ForEachChanged<RigidBody>(since, [&](EntityHandle handle, const RigidBody& rigidBody)
		{
			const auto& entity = entityManager.GetComponent<Entity>(handle);
			uint32_t index = FindRigidBodyIndex(handle);
			if (index != InvalidRecordIndex)
			{
				ReplaceRigidBodyRecord(entity, rigidBody, index);
			}
			else
			{
				const uint32_t slot = entt::to_entity(handle);
				if (slot >= mRigidBodyIndices.size())
				{
					mRigidBodyIndices.resize(slot + 1, InvalidRecordIndex);
				}

				index = static_cast<uint32_t>(mRigidBodies.size());
				mRigidBodyIndices[slot] = index;
				mRigidBodies.push_back(CreateRigidBodyRecord(entity, rigidBody));
			}
			mRigidBodies[index].lastWriteTick = tracker.GetTick();
		});
		mChangeCursor.Commit();
	}

	const auto transforms = tracker.GetTicks<Transform>();
	for (uint32_t index = 0; index < mRigidBodies.size(); ++index)
	{
		auto& record = mRigidBodies[index];
		if (transforms.IsChanged(record.entity, record.lastWriteTick))
		{
			const auto& entity = eastl::as_const(entityManager).GetComponent<Entity>(record.entity);
			const Transform& transform = entity.GetWorldTransform();

			if (transform.scale != record.scale)
			{
				ReplaceRigidBodyRecord(entity, eastl::as_const(entityManager).GetComponent<RigidBody>(record.entity), index);
			}
			else
			{
				RigidBodyPose pose = transform;
				mPhysicsWorld->SetRigidBodyTransform(record.body, pose);
				ResetRigidBodyPose(record, pose);
			}
			record.lastWriteTick = tracker.GetTick();
		}
	}

	mPhysicsWorld->Step(static_cast<float>(Timestep::fixedDeltaTime));
	UpdateRigidBodyPoses();
}

void PhysicsSystem::OnUpdate(EntityManager& entityManager)
{
	const float alpha = static_cast<float>(Timestep::alpha);
	const auto transforms = entityManager.GetChangeTracker().GetTicks<Transform>();
	for (auto& record : mRigidBodies)
	{
		if (record.isInterpolating and not transforms.IsChanged(record.entity, record.lastWriteTick))
		{
			ApplyRigidBodyTransform(entityManager, record, alpha);
			record.isInterpolating = record.previousPose != record.currentPose;
		}
	}
}

void PhysicsSystem::SetGravity(const Float3& gravity)
{
	mGravity = gravity;
	if (mPhysicsWorld)
	{
		mPhysicsWorld->SetGravity(gravity);
	}
}

RigidBodyHandle PhysicsSystem::GetRigidBody(EntityHandle entity) const
{
	const uint32_t index = FindRigidBodyIndex(entity);
	if (index != InvalidRecordIndex)
	{
		return mRigidBodies[index].body;
	}
	else
	{
		return RigidBodyHandle{};
	}
}

uint32_t PhysicsSystem::FindRigidBodyIndex(EntityHandle entity) const
{
	const uint32_t slot = entt::to_entity(entity);
	const uint32_t index = slot < mRigidBodyIndices.size() ? mRigidBodyIndices[slot] : InvalidRecordIndex;
	if (index != InvalidRecordIndex and mRigidBodies[index].entity == entity)
	{
		return index;
	}
	else
	{
		return InvalidRecordIndex;
	}
}

RigidBodyRecord PhysicsSystem::CreateRigidBodyRecord(const Entity& entity, const RigidBody& rigidBody)
{
	const Transform& transform = entity.GetWorldTransform();
	RigidBodyPose pose = transform;

	RigidBodyRecord record;
	record.entity = entity;
	record.body = mPhysicsWorld->CreateRigidBody(entity, rigidBody, pose);
	record.type = rigidBody.type;
	record.scale = transform.scale;
	ResetRigidBodyPose(record, pose);

	float density = 0.0f;
	if (rigidBody.type == RigidBodyType::Static)
	{
		for (const auto& collider : rigidBody.colliders.triangleMeshes)
		{
			const auto& meshes = CreateTriangleMeshes(collider.mesh);
			for (const auto& mesh : meshes)
			{
				mPhysicsWorld->CreateTriangleMeshCollider(record.body, collider, mesh, rigidBody.material, transform.scale);
			}
			record.triangleMeshes.push_back(collider.mesh);
		}
	}
	else if (rigidBody.type == RigidBodyType::Dynamic)
	{
		float volume = 0.0f;
		for (const auto& collider : rigidBody.colliders.boxes)
		{
			volume += mPhysicsWorld->GetColliderVolume(collider, transform.scale);
		}

		for (const auto& collider : rigidBody.colliders.spheres)
		{
			volume += mPhysicsWorld->GetColliderVolume(collider, transform.scale);
		}

		for (const auto& collider : rigidBody.colliders.capsules)
		{
			volume += mPhysicsWorld->GetColliderVolume(collider, transform.scale);
		}

		for (const auto& collider : rigidBody.colliders.convexMeshes)
		{
			const auto& hulls = CreateConvexMeshes(collider.mesh);
			record.convexMeshes.push_back(collider.mesh);

			for (const auto& hull : hulls)
			{
				volume += mPhysicsWorld->GetConvexHullVolume(hull, transform.scale);
			}
		}
		density = volume > 0.0f ? rigidBody.mass / volume : 0.0f;
	}
	else
	{
		for (const auto& collider : rigidBody.colliders.convexMeshes)
		{
			CreateConvexMeshes(collider.mesh);
			record.convexMeshes.push_back(collider.mesh);
		}
	}

	for (const auto& collider : rigidBody.colliders.boxes)
	{
		mPhysicsWorld->CreateBoxCollider(record.body, collider, rigidBody.material, density, transform.scale);
	}

	for (const auto& collider : rigidBody.colliders.spheres)
	{
		mPhysicsWorld->CreateSphereCollider(record.body, collider, rigidBody.material, density, transform.scale);
	}

	for (const auto& collider : rigidBody.colliders.capsules)
	{
		mPhysicsWorld->CreateCapsuleCollider(record.body, collider, rigidBody.material, density, transform.scale);
	}

	if (rigidBody.type != RigidBodyType::Static)
	{
		for (const auto& collider : rigidBody.colliders.convexMeshes)
		{
			for (const auto& hull : mConvexHulls.find(collider.mesh)->second.blobs)
			{
				mPhysicsWorld->CreateConvexMeshCollider(record.body, collider, hull, rigidBody.material, density, transform.scale);
			}
		}
	}

	return record;
}

void PhysicsSystem::ReplaceRigidBodyRecord(const Entity& entity, const RigidBody& rigidBody, uint32_t index)
{
	auto& record = mRigidBodies[index];
	const Float3 linearVelocity = mPhysicsWorld->GetLinearVelocity(record.body);
	const Float3 angularVelocity = mPhysicsWorld->GetAngularVelocity(record.body);

	RigidBodyRecord replacement = CreateRigidBodyRecord(entity, rigidBody);
	DestroyRigidBodyRecord(record);
	record = eastl::move(replacement);

	mPhysicsWorld->SetLinearVelocity(record.body, linearVelocity);
	mPhysicsWorld->SetAngularVelocity(record.body, angularVelocity);
}

void PhysicsSystem::DestroyRigidBodyRecord(const RigidBodyRecord& record)
{
	mPhysicsWorld->DestroyRigidBody(record.body);
	for (const auto& mesh : record.convexMeshes)
	{
		DestroyConvexMesh(mesh);
	}

	for (const auto& mesh : record.triangleMeshes)
	{
		DestroyTriangleMesh(mesh);
	}
}

const TArray<BinaryBuffer>& PhysicsSystem::CreateConvexMeshes(const AssetReference& ref)
{
	auto it = mConvexHulls.find(ref);
	if (it == mConvexHulls.end())
	{
		static auto assetManager = Globals::GameInstance->GetSubsystem<AssetManager>();

		TArray<BinaryBuffer> blobs;
		if (auto mesh = assetManager->Load<Mesh>(ref))
		{
			auto storage = assetManager->GetStorage();
			for (const auto& descriptor : mesh->GetConvexHulls())
			{
				if (const auto blob = mesh->FindBlob<ConvexHullDescriptor>(descriptor.blobSlot, AssetPlatform::Common, AssetUtils::PhysicsBackend()))
				{
					blobs.push_back(storage->ReadBlob(ref, mesh->GetBlobRange(*blob)));
				}
			}
			assetManager->Release(ref);
		}
		it = mConvexHulls.emplace_hint(mConvexHulls.end(), ref, MeshColliderBlobs{ .blobs = blobs });
	}
	++it->second.refCount;
	return it->second.blobs;
}

void PhysicsSystem::DestroyConvexMesh(const AssetReference& ref)
{
	auto it = mConvexHulls.find(ref);
	if (it != mConvexHulls.end() and --it->second.refCount == 0)
	{
		mConvexHulls.erase(it);
	}
}

const TArray<BinaryBuffer>& PhysicsSystem::CreateTriangleMeshes(const AssetReference& ref)
{
	auto it = mTriangleMeshes.find(ref);
	if (it == mTriangleMeshes.end())
	{
		static auto assetManager = Globals::GameInstance->GetSubsystem<AssetManager>();

		TArray<BinaryBuffer> blobs;
		if (auto mesh = assetManager->Load<Mesh>(ref))
		{
			auto storage = assetManager->GetStorage();
			for (const auto& descriptor : mesh->GetTriangleMeshes())
			{
				if (const auto blob = mesh->FindBlob<TriangleMeshDescriptor>(descriptor.blobSlot, AssetPlatform::Common, AssetUtils::PhysicsBackend()))
				{
					blobs.push_back(storage->ReadBlob(ref, mesh->GetBlobRange(*blob)));
				}
			}
			assetManager->Release(ref);
		}
		it = mTriangleMeshes.emplace_hint(mTriangleMeshes.end(), ref, MeshColliderBlobs{ .blobs = blobs });
	}
	++it->second.refCount;
	return it->second.blobs;
}

void PhysicsSystem::DestroyTriangleMesh(const AssetReference& ref)
{
	auto it = mTriangleMeshes.find(ref);
	if (it != mTriangleMeshes.end() and --it->second.refCount == 0)
	{
		mTriangleMeshes.erase(it);
	}
}

void PhysicsSystem::UpdateRigidBodyPoses()
{
	for (auto& record : mRigidBodies)
	{
		record.previousPose = record.currentPose;
	}

	mPhysicsWorld->ForEachRigidBodyMotion([this](const RigidBodyMotion& motion)
	{
		const uint32_t index = FindRigidBodyIndex(motion.entity);
		if (index != InvalidRecordIndex)
		{
			auto& record = mRigidBodies[index];
			record.currentPose = motion.pose;
			record.isInterpolating = true;
		}
	});
}

void PhysicsSystem::ResetRigidBodyPose(RigidBodyRecord& record, const RigidBodyPose& pose)
{
	record.currentPose = pose;
	record.previousPose = record.currentPose;
	record.isInterpolating = false;
}

void PhysicsSystem::ApplyRigidBodyTransform(EntityManager& entityManager, RigidBodyRecord& record, float alpha)
{
	auto& entity = entityManager.GetComponent<Entity>(record.entity);
	const RigidBodyPose pose = Math::Lerp(record.previousPose, record.currentPose, alpha);
	const Transform worldTransform
	{
		.position = pose.position,
		.rotation = pose.rotation,
		.scale = entity.GetWorldScale()
	};

	if (entity.HasParent())
	{
		entity.SetLocalTransform(Math::Inverse(entity.GetParentEntity().GetWorldTransform()) * worldTransform);
	}
	else
	{
		entity.SetLocalTransform(worldTransform);
	}

	record.lastWriteTick = entityManager.GetChangeTracker().GetTick();
	RestoreDynamicDescendants(entityManager, entity, alpha);
}

void PhysicsSystem::RestoreDynamicDescendants(EntityManager& entityManager, const Entity& entity, float alpha)
{
	for (const auto child : entity.GetChildren())
	{
		const uint32_t index = FindRigidBodyIndex(child);
		if (index != InvalidRecordIndex and mRigidBodies[index].type == RigidBodyType::Dynamic)
		{
			ApplyRigidBodyTransform(entityManager, mRigidBodies[index], alpha);
		}
		else
		{
			RestoreDynamicDescendants(entityManager, entityManager.GetComponent<Entity>(child), alpha);
		}
	}
}
