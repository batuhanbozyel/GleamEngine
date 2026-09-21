#include "gpch.h"
#include "PhysicsSystem.h"
#include "Components/Rigidbody.h"
#include "Components/Collider.h"
#include "World/EntityManager.h"

using namespace Gleam;

namespace {

void* ToUserData(EntityHandle entity)
{
	return reinterpret_cast<void*>(static_cast<uintptr_t>(static_cast<uint32_t>(entity)) + 1);
}

EntityHandle FromUserData(void* userData)
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

} // namespace

void PhysicsSystem::OnCreate(EntityManager& entityManager)
{
	mPhysicsWorld = CreateScope<PhysicsWorld>(mGravity);
}

void PhysicsSystem::OnDestroy(EntityManager& entityManager)
{
	mBodies.clear();
	mPhysicsWorld.reset();
}

void PhysicsSystem::OnFixedUpdate(EntityManager& entityManager)
{
	SynchronizeBodies(entityManager);
	mPhysicsWorld->Step(static_cast<float>(Timestep::fixedDeltaTime), subStepCount);
	ApplyBodyMotions(entityManager);
}

void PhysicsSystem::SetGravity(const Float3& gravity)
{
	mGravity = gravity;
	if (mPhysicsWorld)
	{
		mPhysicsWorld->SetGravity(gravity);
	}
}

PhysicsBodyHandle PhysicsSystem::GetBody(EntityHandle entity) const
{
	const auto it = mBodies.find(entity);
	if (it == mBodies.end())
	{
		return PhysicsBodyHandle{};
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
	result.entity = FromUserData(hit.userData);
	result.hit = hit.hit;
	return result;
}

void PhysicsSystem::ForEachContactBegin(ContactFn&& fn) const
{
	mPhysicsWorld->ForEachContactBegin([&fn](const PhysicsContact& contact)
	{
		fn(FromUserData(contact.userDataA), FromUserData(contact.userDataB));
	});
}

void PhysicsSystem::ForEachContactEnd(ContactFn&& fn) const
{
	mPhysicsWorld->ForEachContactEnd([&fn](const PhysicsContact& contact)
	{
		fn(FromUserData(contact.userDataA), FromUserData(contact.userDataB));
	});
}

PhysicsSystem::BodyProxy PhysicsSystem::CreateBodyProxy(EntityManager& entityManager, const Entity& entity, const Rigidbody& rigidbody)
{
	const Transform& transform = entity.GetWorldTransform();
	const float scale = transform.scale;

	PhysicsBodyDescriptor descriptor;
	descriptor.position = transform.position;
	descriptor.rotation = transform.rotation;
	descriptor.type = rigidbody.type;
	descriptor.linearDamping = rigidbody.linearDamping;
	descriptor.angularDamping = rigidbody.angularDamping;
	descriptor.gravityScale = rigidbody.gravityScale;
	descriptor.enableSleep = rigidbody.enableSleep;
	descriptor.isBullet = rigidbody.isBullet;
	descriptor.userData = ToUserData(entity);

	BodyProxy proxy;
	proxy.body = mPhysicsWorld->CreateBody(descriptor);
	proxy.transform = transform;
	proxy.type = rigidbody.type;
	proxy.alive = true;

	bool hasShape = false;
	if (entityManager.HasComponent<BoxCollider>(entity))
	{
		const auto& collider = entityManager.GetComponent<BoxCollider>(entity);

		PhysicsShapeDescriptor shapeDescriptor;
		shapeDescriptor.material = collider.material;
		shapeDescriptor.isSensor = collider.isTrigger;
		shapeDescriptor.userData = descriptor.userData;

		PhysicsBoxShape box;
		box.center = collider.center * scale;
		box.halfExtents = collider.size * (0.5f * scale);

		mPhysicsWorld->CreateBoxShape(proxy.body, shapeDescriptor, box);
		hasShape = true;
	}

	if (entityManager.HasComponent<SphereCollider>(entity))
	{
		const auto& collider = entityManager.GetComponent<SphereCollider>(entity);

		PhysicsShapeDescriptor shapeDescriptor;
		shapeDescriptor.material = collider.material;
		shapeDescriptor.isSensor = collider.isTrigger;
		shapeDescriptor.userData = descriptor.userData;

		PhysicsSphereShape sphere;
		sphere.center = collider.center * scale;
		sphere.radius = collider.radius * scale;

		mPhysicsWorld->CreateSphereShape(proxy.body, shapeDescriptor, sphere);
		hasShape = true;
	}

	if (entityManager.HasComponent<CapsuleCollider>(entity))
	{
		const auto& collider = entityManager.GetComponent<CapsuleCollider>(entity);

		PhysicsShapeDescriptor shapeDescriptor;
		shapeDescriptor.material = collider.material;
		shapeDescriptor.isSensor = collider.isTrigger;
		shapeDescriptor.userData = descriptor.userData;

		PhysicsCapsuleShape capsule;
		capsule.center = collider.center * scale;
		capsule.radius = collider.radius * scale;
		capsule.halfHeight = Math::Max(0.0f, collider.height * 0.5f - collider.radius) * scale;

		mPhysicsWorld->CreateCapsuleShape(proxy.body, shapeDescriptor, capsule);
		hasShape = true;
	}

	if (not hasShape)
	{
		GLEAM_CORE_WARN("PhysicsSystem: entity '{0}' has a Rigidbody but no collider.", entity.GetName());
	}

	return proxy;
}

void PhysicsSystem::SynchronizeBodies(EntityManager& entityManager)
{
	for (auto& [handle, proxy] : mBodies)
	{
		proxy.alive = false;
	}

	entityManager.ForEach<Entity, Rigidbody>([&](EntityHandle handle, const Entity& entity, const Rigidbody& rigidbody)
	{
		auto it = mBodies.find(handle);
		if (it == mBodies.end())
		{
			mBodies.emplace(handle, CreateBodyProxy(entityManager, entity, rigidbody));
		}
		else
		{
			BodyProxy& proxy = it->second;
			const Transform& transform = entity.GetWorldTransform();

			if (transform.scale != proxy.transform.scale)
			{
				mPhysicsWorld->DestroyBody(proxy.body);
				proxy = CreateBodyProxy(entityManager, entity, rigidbody);
			}
			else
			{
				if (proxy.type != rigidbody.type)
				{
					mPhysicsWorld->SetBodyType(proxy.body, rigidbody.type);
					proxy.type = rigidbody.type;
				}

				if (transform.position != proxy.transform.position or transform.rotation != proxy.transform.rotation)
				{
					mPhysicsWorld->SetBodyTransform(proxy.body, transform.position, transform.rotation);
					proxy.transform = transform;
				}
			}
			proxy.alive = true;
		}
	});

	for (auto it = mBodies.begin(); it != mBodies.end();)
	{
		if (it->second.alive)
		{
			++it;
		}
		else
		{
			mPhysicsWorld->DestroyBody(it->second.body);
			it = mBodies.erase(it);
		}
	}
}

void PhysicsSystem::ApplyBodyMotions(EntityManager& entityManager)
{
	mPhysicsWorld->ForEachBodyMotion([&](const PhysicsBodyMotion& motion)
	{
		const EntityHandle handle = FromUserData(motion.userData);
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

		auto it = mBodies.find(handle);
		if (it != mBodies.end())
		{
			it->second.transform = entity.GetWorldTransform();
		}
	});
}
