#pragma once
#include "PhysicsTypes.h"
#include "Collider.h"
#include "World/Components/RigidBody.h"
#include "Container/BinaryBuffer.h"

#include <functional>

namespace Gleam {

class PhysicsSystem;

class PhysicsWorld final
{
	using MotionFn = std::function<void(const RigidBodyMotion&)>;
	using ContactFn = std::function<void(const PhysicsContact&)>;

	friend class PhysicsSystem;
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

	RigidBodyHandle CreateRigidBody(EntityHandle entity, const RigidBody& rigidBody, const RigidBodyPose& pose);

	ColliderHandle CreateSphereCollider(RigidBodyHandle body, const SphereCollider& collider, const PhysicsMaterial& material, float density, float scale);

	ColliderHandle CreateBoxCollider(RigidBodyHandle body, const BoxCollider& collider, const PhysicsMaterial& material, float density, float scale);

	ColliderHandle CreateCapsuleCollider(RigidBodyHandle body, const CapsuleCollider& collider, const PhysicsMaterial& material, float density, float scale);

	ColliderHandle CreateConvexMeshCollider(RigidBodyHandle body, const ConvexMeshCollider& collider, const BinaryBuffer& hull, const PhysicsMaterial& material, float density, float scale);

	ColliderHandle CreateTriangleMeshCollider(RigidBodyHandle body, const TriangleMeshCollider& collider, const BinaryBuffer& mesh, const PhysicsMaterial& material, float scale);

	void DestroyRigidBody(RigidBodyHandle body);

	void DestroyCollider(ColliderHandle collider);

	void SetRigidBodyTransform(RigidBodyHandle body, const RigidBodyPose& pose);

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

	float GetColliderVolume(const BoxCollider& collider, float scale) const;

	float GetColliderVolume(const SphereCollider& collider, float scale) const;

	float GetColliderVolume(const CapsuleCollider& collider, float scale) const;

	float GetConvexHullVolume(const BinaryBuffer& hull, float scale) const;

private:

	NativePhysicsHandle mWorldHandle = 0;

};

} // namespace Gleam
