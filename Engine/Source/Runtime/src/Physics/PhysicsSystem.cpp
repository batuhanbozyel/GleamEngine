#include "gpch.h"
#include "PhysicsSystem.h"
#include "Components/RigidBody.h"
#include "Collider.h"
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
}

void PhysicsSystem::OnDestroy(EntityManager& entityManager)
{
	mRigidBodies.clear();
	mPhysicsWorld.reset();
}

void PhysicsSystem::OnFixedUpdate(EntityManager& entityManager)
{
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

PhysicsSystem::RigidBodyProxy PhysicsSystem::CreateRigidBodyProxy(const Entity& entity, const RigidBody& rigidBody)
{
	const Transform& transform = entity.GetWorldTransform();
	const float scale = transform.scale;

	void* userData = PhysicsUtils::ToUserData(entity);

	RigidBodyProxy proxy;
	proxy.body = mPhysicsWorld->CreateRigidBody(rigidBody, transform.position, transform.rotation, userData);
	proxy.transform = transform;
	proxy.type = rigidBody.type;
	proxy.alive = true;

	float volume = 0.0f;
	for (const auto& collider : rigidBody.colliders.boxes)
	{
		volume += PhysicsUtils::ComputeVolume(collider, scale);
	}

	for (const auto& collider : rigidBody.colliders.spheres)
	{
		volume += PhysicsUtils::ComputeVolume(collider, scale);
	}

	for (const auto& collider : rigidBody.colliders.capsules)
	{
		volume += PhysicsUtils::ComputeVolume(collider, scale);
	}

	const float density = volume > 0.0f ? rigidBody.mass / volume : 0.0f;
	for (const auto& collider : rigidBody.colliders.boxes)
	{
		mPhysicsWorld->CreateBoxCollider(proxy.body, collider, rigidBody.material, density, scale, userData);
	}

	for (const auto& collider : rigidBody.colliders.spheres)
	{
		mPhysicsWorld->CreateSphereCollider(proxy.body, collider, rigidBody.material, density, scale, userData);
	}

	for (const auto& collider : rigidBody.colliders.capsules)
	{
		mPhysicsWorld->CreateCapsuleCollider(proxy.body, collider, rigidBody.material, density, scale, userData);
	}

	return proxy;
}

void PhysicsSystem::SynchronizeRigidBodies(EntityManager& entityManager)
{
	for (auto& [handle, proxy] : mRigidBodies)
	{
		proxy.alive = false;
	}

	entityManager.ForEach<Entity, RigidBody>([&](EntityHandle handle, const Entity& entity, const RigidBody& rigidBody)
	{
		auto it = mRigidBodies.find(handle);
		if (it == mRigidBodies.end())
		{
			mRigidBodies.emplace(handle, CreateRigidBodyProxy(entity, rigidBody));
		}
		else
		{
			RigidBodyProxy& proxy = it->second;
			const Transform& transform = entity.GetWorldTransform();

			if (transform.scale != proxy.transform.scale)
			{
				mPhysicsWorld->DestroyRigidBody(proxy.body);
				proxy = CreateRigidBodyProxy(entity, rigidBody);
			}
			else
			{
				if (proxy.type != rigidBody.type)
				{
					mPhysicsWorld->SetRigidBodyType(proxy.body, rigidBody.type);
					proxy.type = rigidBody.type;
				}

				if (transform.position != proxy.transform.position or transform.rotation != proxy.transform.rotation)
				{
					mPhysicsWorld->SetRigidBodyTransform(proxy.body, transform.position, transform.rotation);
					proxy.transform = transform;
				}
			}
			proxy.alive = true;
		}
	});

	for (auto it = mRigidBodies.begin(); it != mRigidBodies.end();)
	{
		if (it->second.alive)
		{
			++it;
		}
		else
		{
			mPhysicsWorld->DestroyRigidBody(it->second.body);
			it = mRigidBodies.erase(it);
		}
	}
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
