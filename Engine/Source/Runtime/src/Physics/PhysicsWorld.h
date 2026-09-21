#pragma once
#include "PhysicsTypes.h"

#include <functional>

namespace Gleam {

class PhysicsWorld final
{
	using MotionFn = std::function<void(const PhysicsBodyMotion&)>;
	using ContactFn = std::function<void(const PhysicsContact&)>;
public:

	PhysicsWorld();

	explicit PhysicsWorld(const Float3& gravity);

	~PhysicsWorld();

	PhysicsWorld(PhysicsWorld&& other) noexcept;

	PhysicsWorld& operator=(PhysicsWorld&& other) noexcept;

	PhysicsWorld(const PhysicsWorld&) = delete;

	PhysicsWorld& operator=(const PhysicsWorld&) = delete;

	void Step(float deltaTime, uint32_t subStepCount);

	void SetGravity(const Float3& gravity);

	Float3 GetGravity() const;

	PhysicsBodyHandle CreateBody(const PhysicsBodyDescriptor& descriptor);

	void DestroyBody(PhysicsBodyHandle body);

	bool IsValidBody(PhysicsBodyHandle body) const;

	PhysicsShapeHandle CreateSphereShape(PhysicsBodyHandle body, const PhysicsShapeDescriptor& descriptor, const PhysicsSphereShape& sphere);

	PhysicsShapeHandle CreateBoxShape(PhysicsBodyHandle body, const PhysicsShapeDescriptor& descriptor, const PhysicsBoxShape& box);

	PhysicsShapeHandle CreateCapsuleShape(PhysicsBodyHandle body, const PhysicsShapeDescriptor& descriptor, const PhysicsCapsuleShape& capsule);

	void DestroyShape(PhysicsShapeHandle shape);

	void SetBodyType(PhysicsBodyHandle body, PhysicsBodyType type);

	void SetBodyTransform(PhysicsBodyHandle body, const Float3& position, const Quaternion& rotation);

	Float3 GetBodyPosition(PhysicsBodyHandle body) const;

	Quaternion GetBodyRotation(PhysicsBodyHandle body) const;

	void SetLinearVelocity(PhysicsBodyHandle body, const Float3& velocity);

	Float3 GetLinearVelocity(PhysicsBodyHandle body) const;

	void SetAngularVelocity(PhysicsBodyHandle body, const Float3& velocity);

	Float3 GetAngularVelocity(PhysicsBodyHandle body) const;

	void SetLinearDamping(PhysicsBodyHandle body, float damping);

	void SetAngularDamping(PhysicsBodyHandle body, float damping);

	void SetGravityScale(PhysicsBodyHandle body, float scale);

	void ApplyForce(PhysicsBodyHandle body, const Float3& force, bool wake = true);

	void ApplyForceAtPoint(PhysicsBodyHandle body, const Float3& force, const Float3& point, bool wake = true);

	void ApplyTorque(PhysicsBodyHandle body, const Float3& torque, bool wake = true);

	void ApplyLinearImpulse(PhysicsBodyHandle body, const Float3& impulse, bool wake = true);

	void ApplyAngularImpulse(PhysicsBodyHandle body, const Float3& impulse, bool wake = true);

	void SetBodyAwake(PhysicsBodyHandle body, bool awake);

	bool IsBodyAwake(PhysicsBodyHandle body) const;

	PhysicsRaycastHit Raycast(const Float3& origin, const Float3& direction, float distance) const;

	void ForEachBodyMotion(MotionFn&& fn) const;

	void ForEachContactBegin(ContactFn&& fn) const;

	void ForEachContactEnd(ContactFn&& fn) const;

	bool IsValid() const;

private:

	NativePhysicsHandle mWorldHandle = 0;

};

} // namespace Gleam
