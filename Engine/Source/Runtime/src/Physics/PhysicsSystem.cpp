#include "gpch.h"
#include "PhysicsSystem.h"
#include "Collider.h"
#include "World/Components/RigidBody.h"
#include "World/EntityManager.h"

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
		mPhysicsWorld->DestroyRigidBody(it->second.body);
		mRigidBodies.erase(it);
	}
}

void PhysicsSystem::OnDestroy(EntityManager& entityManager)
{
	mRigidBodyRemoved.Reset();
	mRigidBodies.clear();
	mPhysicsWorld.reset();
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
	record.transform = transform;

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

	return record;
}

void PhysicsSystem::ReplaceRigidBodyRecord(const Entity& entity, const RigidBody& rigidBody, RigidBodyRecord& record)
{
	const Float3 linearVelocity = mPhysicsWorld->GetLinearVelocity(record.body);
	const Float3 angularVelocity = mPhysicsWorld->GetAngularVelocity(record.body);

	mPhysicsWorld->DestroyRigidBody(record.body);
	record = CreateRigidBodyRecord(entity, rigidBody);

	mPhysicsWorld->SetLinearVelocity(record.body, linearVelocity);
	mPhysicsWorld->SetAngularVelocity(record.body, angularVelocity);
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
	mPhysicsWorld->ForEachRigidBodyMotion([&](const RigidBodyMotion& motion)
	{
		const EntityHandle handle = PhysicsUtils::FromUserData(motion.userData);
		if (not entityManager.IsValid(handle))
		{
			return;
		}

		auto& entity = entityManager.GetComponent<Entity>(handle);
		const Transform worldTransform
		{
			.position = motion.position,
			.rotation = motion.rotation,
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

		auto it = mRigidBodies.find(handle);
		if (it != mRigidBodies.end())
		{
			it->second.transform = entity.GetWorldTransform();
		}
	});
}
