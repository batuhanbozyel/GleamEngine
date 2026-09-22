#pragma once
#include "PhysicsTypes.h"
#include "Components/RigidBody.h"
#include "Collider.h"

#include <functional>

namespace Gleam {

class PhysicsWorld final
{
	using MotionFn = std::function<void(const RigidBodyMotion&)>;
	using ContactFn = std::function<void(const PhysicsContact&)>;
public:

	PhysicsWorld();

	explicit PhysicsWorld(const Float3& gravity);

	~PhysicsWorld();

	PhysicsWorld(PhysicsWorld&& other) noexcept;

	PhysicsWorld& operator=(PhysicsWorld&& other) noexcept;

	PhysicsWorld(const PhysicsWorld&) = delete;

	PhysicsWorld& operator=(const PhysicsWorld&) = delete;

	void Step(float deltaTime);

	void SetGravity(const Float3& gravity);

	Float3 GetGravity() const;

	RigidBodyHandle CreateRigidBody(const RigidBody& rigidBody, const Float3& position, const Quaternion& rotation, void* userData = nullptr);

	void DestroyRigidBody(RigidBodyHandle body);

	bool IsValidRigidBody(RigidBodyHandle body) const;

	ColliderHandle CreateSphereCollider(RigidBodyHandle body, const SphereCollider& collider, const PhysicsMaterial& material, float density, float scale, void* userData = nullptr);

	ColliderHandle CreateBoxCollider(RigidBodyHandle body, const BoxCollider& collider, const PhysicsMaterial& material, float density, float scale, void* userData = nullptr);

	ColliderHandle CreateCapsuleCollider(RigidBodyHandle body, const CapsuleCollider& collider, const PhysicsMaterial& material, float density, float scale, void* userData = nullptr);

	void DestroyCollider(ColliderHandle collider);

	void SetRigidBodyType(RigidBodyHandle body, RigidBodyType type);

	void SetRigidBodyTransform(RigidBodyHandle body, const Float3& position, const Quaternion& rotation);

	Float3 GetRigidBodyPosition(RigidBodyHandle body) const;

	Quaternion GetRigidBodyRotation(RigidBodyHandle body) const;

	void SetLinearVelocity(RigidBodyHandle body, const Float3& velocity);

	Float3 GetLinearVelocity(RigidBodyHandle body) const;

	void SetAngularVelocity(RigidBodyHandle body, const Float3& velocity);

	Float3 GetAngularVelocity(RigidBodyHandle body) const;

	void SetLinearDamping(RigidBodyHandle body, float damping);

	void SetAngularDamping(RigidBodyHandle body, float damping);

	void SetGravityScale(RigidBodyHandle body, float scale);

	void ApplyForce(RigidBodyHandle body, const Float3& force, bool wake = true);

	void ApplyForceAtPoint(RigidBodyHandle body, const Float3& force, const Float3& point, bool wake = true);

	void ApplyTorque(RigidBodyHandle body, const Float3& torque, bool wake = true);

	void ApplyLinearImpulse(RigidBodyHandle body, const Float3& impulse, bool wake = true);

	void ApplyAngularImpulse(RigidBodyHandle body, const Float3& impulse, bool wake = true);

	void SetRigidBodyAwake(RigidBodyHandle body, bool awake);

	bool IsRigidBodyAwake(RigidBodyHandle body) const;

	PhysicsRaycastHit Raycast(const Float3& origin, const Float3& direction, float distance) const;

	void ForEachRigidBodyMotion(MotionFn&& fn) const;

	void ForEachContactBegin(ContactFn&& fn) const;

	void ForEachContactEnd(ContactFn&& fn) const;

	bool IsValid() const;

private:

	NativePhysicsHandle mWorldHandle = 0;

};

} // namespace Gleam
