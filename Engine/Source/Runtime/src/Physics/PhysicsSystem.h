#pragma once
#include "PhysicsWorld.h"
#include "World/ComponentSystem.h"
#include "World/Entity.h"
#include "Container/Hash.h"
#include "Container/Pointer.h"

#include <functional>

namespace Gleam {

struct Rigidbody;

struct PhysicsRaycastResult
{
	Float3 point = Float3::zero;
	Float3 normal = Float3::zero;
	float distance = 0.0f;
	EntityHandle entity = InvalidEntity;
	bool hit = false;
};

class PhysicsSystem final : public ComponentSystem
{
	using ContactFn = std::function<void(EntityHandle, EntityHandle)>;
public:

	virtual void OnCreate(EntityManager& entityManager) override;

	virtual void OnFixedUpdate(EntityManager& entityManager) override;

	virtual void OnDestroy(EntityManager& entityManager) override;

	PhysicsWorld& GetPhysicsWorld()
	{
		return *mPhysicsWorld;
	}

	const PhysicsWorld& GetPhysicsWorld() const
	{
		return *mPhysicsWorld;
	}

	void SetGravity(const Float3& gravity);

	Float3 GetGravity() const
	{
		return mGravity;
	}

	PhysicsBodyHandle GetBody(EntityHandle entity) const;

	PhysicsRaycastResult Raycast(const Float3& origin, const Float3& direction, float distance) const;

	void ForEachContactBegin(ContactFn&& fn) const;

	void ForEachContactEnd(ContactFn&& fn) const;

	uint32_t subStepCount = 4;

private:

	struct BodyProxy
	{
		PhysicsBodyHandle body = {};
		Transform transform = {};
		PhysicsBodyType type = PhysicsBodyType::Dynamic;
		bool alive = false;
	};

	void SynchronizeBodies(EntityManager& entityManager);

	void ApplyBodyMotions(EntityManager& entityManager);

	BodyProxy CreateBodyProxy(EntityManager& entityManager, const Entity& entity, const Rigidbody& rigidbody);

	Scope<PhysicsWorld> mPhysicsWorld;

	HashMap<EntityHandle, BodyProxy, EnumClassHash> mBodies;

	Float3 mGravity = Float3{ 0.0f, -9.81f, 0.0f };

};

} // namespace Gleam
