#pragma once
#include "PhysicsWorld.h"
#include "World/ComponentSystem.h"
#include "World/Entity.h"
#include "Container/Hash.h"
#include "Container/Pointer.h"

namespace Gleam {

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

	RigidBodyHandle GetRigidBody(EntityHandle entity) const;

	PhysicsRaycastResult Raycast(const Float3& origin, const Float3& direction, float distance) const;

	void ForEachContactBegin(ContactFn&& fn) const;

	void ForEachContactEnd(ContactFn&& fn) const;

private:

	struct RigidBodyProxy
	{
		RigidBodyHandle body = {};
		Transform transform = {};
		RigidBodyType type = RigidBodyType::Dynamic;
		bool alive = false;
	};

	void SynchronizeRigidBodies(EntityManager& entityManager);

	void ApplyRigidBodyMotions(EntityManager& entityManager);

	RigidBodyProxy CreateRigidBodyProxy(const Entity& entity, const RigidBody& rigidBody);

	Scope<PhysicsWorld> mPhysicsWorld;

	HashMap<EntityHandle, RigidBodyProxy, EnumClassHash> mRigidBodies;

	Float3 mGravity = Float3{ 0.0f, -9.81f, 0.0f };

};

} // namespace Gleam
