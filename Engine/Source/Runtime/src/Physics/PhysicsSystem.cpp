#include "gpch.h"
#include "PhysicsSystem.h"
#include "Collider.h"
#include "World/Components/RigidBody.h"
#include "World/EntityManager.h"
#include "Assets/AssetManager.h"
#include "Renderer/Mesh.h"

using namespace Gleam;

namespace PhysicsUtils {

static void* ToUserData(EntityHandle entity)
{
	return reinterpret_cast<void*>(static_cast<uintptr_t>(static_cast<uint32_t>(entity)) + 1);
}

static EntityHandle FromUserData(void* userData)
{
	const auto value = reinterpret_cast<uintptr_t>(userData);
	if (value == 0)
	{
		return InvalidEntity;
	}
	else
	{
		return static_cast<EntityHandle>(static_cast<uint32_t>(value - 1));
	}
}

static float ComputeVolume(const BoxCollider& collider, float scale)
{
	const Float3 size = collider.size * scale;
	return Math::Abs(size.x * size.y * size.z);
}

static float ComputeVolume(const SphereCollider& collider, float scale)
{
	const float radius = collider.radius * scale;
	return (4.0f / 3.0f) * Math::PI * radius * radius * radius;
}

static float ComputeVolume(const CapsuleCollider& collider, float scale)
{
	const float radius = collider.radius * scale;
	const float segment = Math::Max(0.0f, collider.height - 2.0f * collider.radius) * scale;
	const float cylinder = Math::PI * radius * radius * segment;
	const float caps = (4.0f / 3.0f) * Math::PI * radius * radius * radius;
	return cylinder + caps;
}

static float ComputeVolume(const BinaryBuffer& hull, float scale)
{
	return PhysicsWorld::GetConvexHullVolume(hull) * scale * scale * scale;
}

template<typename Descriptor>
static TArray<BinaryBuffer> LoadColliderBlobs(const AssetReference& reference, const TArray<Descriptor>& (Mesh::*descriptors)() const)
{
	static auto assetManager = Globals::GameInstance->GetSubsystem<AssetManager>();

	TArray<BinaryBuffer> blobs;
	if (auto mesh = assetManager->Load<Mesh>(reference))
	{
		auto storage = assetManager->GetStorage();
		for (const auto& descriptor : (mesh->*descriptors)())
		{
			if (const auto blob = mesh->FindBlob<Descriptor>(descriptor.blobSlot, AssetPlatform::Common, AssetUtils::PhysicsBackend()))
			{
				blobs.push_back(storage->ReadBlob(reference, mesh->GetBlobRange(*blob)));
			}
		}
		assetManager->Release(reference);
	}
	return blobs;
}

static const TArray<BinaryBuffer>& AcquireColliders(MeshColliderCache& cache, const AssetReference& mesh, TArray<BinaryBuffer>(*load)(const AssetReference&))
{
	auto it = cache.find(mesh);
	if (it == cache.end())
	{
		it = cache.emplace(mesh, MeshColliderBlobs{ .blobs = load(mesh) }).first;
	}
	++it->second.refCount;
	return it->second.blobs;
}

static void ReleaseColliders(MeshColliderCache& cache, const AssetReference& mesh)
{
	auto it = cache.find(mesh);
	if (it != cache.end() and --it->second.refCount == 0)
	{
		cache.erase(it);
	}
}

} // namespace PhysicsUtils

void PhysicsSystem::OnCreate(EntityManager& entityManager)
{
	mPhysicsWorld = CreateScope<PhysicsWorld>(mGravity);
	mRigidBodyRemoved = entityManager.OnComponentRemoved<RigidBody, &PhysicsSystem::OnRigidBodyRemoved>(*this);
}

void PhysicsSystem::OnRigidBodyRemoved(EntityHandle entity)
{
	const auto it = mRigidBodies.find(entity);
	if (it != mRigidBodies.end())
	{
		DestroyRigidBodyRecord(it->second);
		mRigidBodies.erase(it);
	}
}

void PhysicsSystem::OnDestroy(EntityManager& entityManager)
{
	mRigidBodyRemoved.Reset();
	mRigidBodies.clear();
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
			auto it = mRigidBodies.find(handle);
			if (it == mRigidBodies.end())
			{
				mRigidBodies.emplace(handle, CreateRigidBodyRecord(entity, rigidBody));
			}
			else
			{
				ReplaceRigidBodyRecord(entity, rigidBody, it->second);
			}
		});
		mChangeCursor.Commit();
	}

	SynchronizeRigidBodies(entityManager);
	mPhysicsWorld->Step(static_cast<float>(Timestep::fixedDeltaTime));
	ApplyRigidBodyMotions(entityManager);
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
	const auto it = mRigidBodies.find(entity);
	if (it == mRigidBodies.end())
	{
		return RigidBodyHandle{};
	}
	else
	{
		return it->second.body;
	}
}

PhysicsRaycastResult PhysicsSystem::Raycast(const Float3& origin, const Float3& direction, float distance) const
{
	const PhysicsRaycastHit hit = mPhysicsWorld->Raycast(origin, direction, distance);

	PhysicsRaycastResult result;
	result.point = hit.point;
	result.normal = hit.normal;
	result.distance = hit.distance;
	result.entity = PhysicsUtils::FromUserData(hit.userData);
	result.hit = hit.hit;
	return result;
}

void PhysicsSystem::ForEachContactBegin(ContactFn&& fn) const
{
	mPhysicsWorld->ForEachContactBegin([&fn](const PhysicsContact& contact)
	{
		fn(PhysicsUtils::FromUserData(contact.userDataA), PhysicsUtils::FromUserData(contact.userDataB));
	});
}

void PhysicsSystem::ForEachContactEnd(ContactFn&& fn) const
{
	mPhysicsWorld->ForEachContactEnd([&fn](const PhysicsContact& contact)
	{
		fn(PhysicsUtils::FromUserData(contact.userDataA), PhysicsUtils::FromUserData(contact.userDataB));
	});
}

RigidBodyRecord PhysicsSystem::CreateRigidBodyRecord(const Entity& entity, const RigidBody& rigidBody)
{
	const Transform& transform = entity.GetWorldTransform();
	void* userData = PhysicsUtils::ToUserData(entity);

	RigidBodyRecord record;
	record.body = mPhysicsWorld->CreateRigidBody(rigidBody, transform.position, transform.rotation, userData);
	record.type = rigidBody.type;
	record.transform = transform;

	const bool supportsConvexMeshes = rigidBody.type != RigidBodyType::Static;
	const bool supportsTriangleMeshes = rigidBody.type != RigidBodyType::Dynamic;
	if (not supportsConvexMeshes and not rigidBody.colliders.convexMeshes.empty())
	{
		GLEAM_CORE_WARN("Convex mesh colliders require a kinematic or dynamic rigid body: {0}", entity.GetName());
	}

	if (not supportsTriangleMeshes and not rigidBody.colliders.triangleMeshes.empty())
	{
		GLEAM_CORE_WARN("Triangle mesh colliders require a static or kinematic rigid body: {0}", entity.GetName());
	}

	float volume = 0.0f;
	for (const auto& collider : rigidBody.colliders.boxes)
	{
		volume += PhysicsUtils::ComputeVolume(collider, transform.scale);
	}

	for (const auto& collider : rigidBody.colliders.spheres)
	{
		volume += PhysicsUtils::ComputeVolume(collider, transform.scale);
	}

	for (const auto& collider : rigidBody.colliders.capsules)
	{
		volume += PhysicsUtils::ComputeVolume(collider, transform.scale);
	}

	if (supportsConvexMeshes)
	{
		for (const auto& collider : rigidBody.colliders.convexMeshes)
		{
			const auto& hulls = PhysicsUtils::AcquireColliders(mConvexHulls, collider.mesh, &PhysicsSystem::LoadConvexHulls);
			record.convexMeshes.push_back(collider.mesh);
			for (const auto& hull : hulls)
			{
				volume += PhysicsUtils::ComputeVolume(hull, transform.scale);
			}
		}
	}

	if (supportsTriangleMeshes)
	{
		for (const auto& collider : rigidBody.colliders.triangleMeshes)
		{
			PhysicsUtils::AcquireColliders(mTriangleMeshes, collider.mesh, &PhysicsSystem::LoadTriangleMeshes);
			record.triangleMeshes.push_back(collider.mesh);
		}
	}

	const float density = volume > 0.0f ? rigidBody.mass / volume : 0.0f;
	for (const auto& collider : rigidBody.colliders.boxes)
	{
		mPhysicsWorld->CreateBoxCollider(record.body, collider, rigidBody.material, density, transform.scale, userData);
	}

	for (const auto& collider : rigidBody.colliders.spheres)
	{
		mPhysicsWorld->CreateSphereCollider(record.body, collider, rigidBody.material, density, transform.scale, userData);
	}

	for (const auto& collider : rigidBody.colliders.capsules)
	{
		mPhysicsWorld->CreateCapsuleCollider(record.body, collider, rigidBody.material, density, transform.scale, userData);
	}

	if (supportsConvexMeshes)
	{
		for (const auto& collider : rigidBody.colliders.convexMeshes)
		{
			for (const auto& hull : mConvexHulls.find(collider.mesh)->second.blobs)
			{
				mPhysicsWorld->CreateConvexMeshCollider(record.body, collider, hull, rigidBody.material, density, transform.scale, userData);
			}
		}
	}

	if (supportsTriangleMeshes)
	{
		for (const auto& collider : rigidBody.colliders.triangleMeshes)
		{
			for (const auto& mesh : mTriangleMeshes.find(collider.mesh)->second.blobs)
			{
				mPhysicsWorld->CreateTriangleMeshCollider(record.body, collider, mesh, rigidBody.material, transform.scale, userData);
			}
		}
	}

	return record;
}

void PhysicsSystem::ReplaceRigidBodyRecord(const Entity& entity, const RigidBody& rigidBody, RigidBodyRecord& record)
{
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
		PhysicsUtils::ReleaseColliders(mConvexHulls, mesh);
	}

	for (const auto& mesh : record.triangleMeshes)
	{
		PhysicsUtils::ReleaseColliders(mTriangleMeshes, mesh);
	}
}

TArray<BinaryBuffer> PhysicsSystem::LoadConvexHulls(const AssetReference& mesh)
{
	return PhysicsUtils::LoadColliderBlobs<ConvexHullDescriptor>(mesh, &Mesh::GetConvexHulls);
}

TArray<BinaryBuffer> PhysicsSystem::LoadTriangleMeshes(const AssetReference& mesh)
{
	return PhysicsUtils::LoadColliderBlobs<TriangleMeshDescriptor>(mesh, &Mesh::GetTriangleMeshes);
}

void PhysicsSystem::SynchronizeRigidBodies(const EntityManager& entityManager)
{
	const auto& tracker = entityManager.GetChangeTracker();
	const Tick since = mTransformCursor.Begin(tracker);
	const auto transforms = tracker.GetTicks<Transform>();

	for (auto& [handle, record] : mRigidBodies)
	{
		if (transforms.IsChanged(handle, since))
		{
			const auto& entity = entityManager.GetComponent<Entity>(handle);
			const Transform& transform = entity.GetWorldTransform();

			if (transform.scale != record.transform.scale)
			{
				ReplaceRigidBodyRecord(entity, entityManager.GetComponent<RigidBody>(handle), record);
			}
			else if (transform.position != record.transform.position or transform.rotation != record.transform.rotation)
			{
				mPhysicsWorld->SetRigidBodyTransform(record.body, transform.position, transform.rotation);
				record.transform = transform;
			}
		}
	}

	mTransformCursor.Commit();
}

void PhysicsSystem::ApplyRigidBodyMotions(EntityManager& entityManager)
{
	TArray<EntityHandle> movedEntities;
	mPhysicsWorld->ForEachRigidBodyMotion([&](const RigidBodyMotion& motion)
	{
		const EntityHandle handle = PhysicsUtils::FromUserData(motion.userData);
		auto it = mRigidBodies.find(handle);
		if (entityManager.IsValid(handle) and it != mRigidBodies.end())
		{
			it->second.transform.position = motion.position;
			it->second.transform.rotation = motion.rotation;
			movedEntities.push_back(handle);
		}
	});

	for (const auto handle : movedEntities)
	{
		ApplyRigidBodyTransform(entityManager, handle);
	}
}

void PhysicsSystem::ApplyRigidBodyTransform(EntityManager& entityManager, EntityHandle handle)
{
	auto& entity = entityManager.GetComponent<Entity>(handle);
	auto& record = mRigidBodies.find(handle)->second;
	const Transform worldTransform
	{
		.position = record.transform.position,
		.rotation = record.transform.rotation,
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

	record.transform = entity.GetWorldTransform();
	RestoreDynamicDescendants(entityManager, entity);
}

void PhysicsSystem::RestoreDynamicDescendants(EntityManager& entityManager, const Entity& entity)
{
	for (const auto child : entity.GetChildren())
	{
		const auto it = mRigidBodies.find(child);
		if (it != mRigidBodies.end() and it->second.type == RigidBodyType::Dynamic)
		{
			ApplyRigidBodyTransform(entityManager, child);
		}
		else
		{
			RestoreDynamicDescendants(entityManager, entityManager.GetComponent<Entity>(child));
		}
	}
}
