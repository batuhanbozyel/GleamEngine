#include "gpch.h"
#include "PhysicsWorld.h"

#include <box3d/box3d.h>
#include <cstring>

using namespace Gleam;

namespace PhysicsUtils {

static_assert(sizeof(b3WorldId) <= sizeof(uint64_t), "b3WorldId does not fit in a PhysicsWorld handle.");
static_assert(sizeof(b3BodyId) <= sizeof(uint64_t), "b3BodyId does not fit in a RigidBodyHandle.");
static_assert(sizeof(b3ShapeId) <= sizeof(uint64_t), "b3ShapeId does not fit in a ColliderHandle.");
static_assert(sizeof(Float3) == sizeof(b3Vec3), "Float3 must match b3Vec3 to alias mesh vertices without a copy.");
static_assert(alignof(Float3) == alignof(b3Vec3), "Float3 must match b3Vec3 alignment to alias mesh vertices without a copy.");
static_assert(sizeof(uint32_t) == sizeof(int32_t), "Mesh indices must match the width box3d expects.");

static b3WorldId ToWorldId(uint64_t handle)
{
	b3WorldId id = {};
	std::memcpy(&id, &handle, sizeof(id));
	return id;
}

static uint64_t FromWorldId(b3WorldId id)
{
	uint64_t handle = 0;
	std::memcpy(&handle, &id, sizeof(id));
	return handle;
}

static b3BodyId ToBox3D(RigidBodyHandle handle)
{
	b3BodyId id = {};
	std::memcpy(&id, &handle.value, sizeof(id));
	return id;
}

static RigidBodyHandle FromBox3D(b3BodyId id)
{
	RigidBodyHandle handle;
	std::memcpy(&handle.value, &id, sizeof(id));
	return handle;
}

static b3ShapeId ToBox3D(ColliderHandle handle)
{
	b3ShapeId id = {};
	std::memcpy(&id, &handle.value, sizeof(id));
	return id;
}

static ColliderHandle FromBox3D(b3ShapeId id)
{
	ColliderHandle handle;
	std::memcpy(&handle.value, &id, sizeof(id));
	return handle;
}

static b3Vec3 ToBox3D(const Float3& vec)
{
	return b3Vec3{ vec.x, vec.y, vec.z };
}

static Float3 FromBox3D(const b3Vec3& vec)
{
	return Float3{ vec.x, vec.y, vec.z };
}

static b3Quat ToBox3D(const Quaternion& quat)
{
	return b3Quat{ { quat.x, quat.y, quat.z }, quat.w };
}

static Quaternion FromBox3D(const b3Quat& quat)
{
	return Quaternion{ quat.s, quat.v.x, quat.v.y, quat.v.z };
}

static b3BodyType ToBox3D(RigidBodyType type)
{
	if (type == RigidBodyType::Static)
	{
		return b3_staticBody;
	}
	else if (type == RigidBodyType::Kinematic)
	{
		return b3_kinematicBody;
	}
	else
	{
		return b3_dynamicBody;
	}
}

static b3ShapeDef MakeShapeDef(const PhysicsMaterial& material, float density, bool isTrigger, void* userData)
{
	b3ShapeDef def = b3DefaultShapeDef();
	def.density = density;
	def.baseMaterial.friction = material.friction;
	def.baseMaterial.restitution = material.restitution;
	def.baseMaterial.rollingResistance = material.rollingResistance;
	def.isSensor = isTrigger;
	def.enableSensorEvents = isTrigger;
	def.enableContactEvents = true;
	def.userData = userData;
	return def;
}

static void* GetShapeBodyUserData(b3ShapeId shape)
{
	if (b3Shape_IsValid(shape))
	{
		return b3Body_GetUserData(b3Shape_GetBody(shape));
	}
	else
	{
		return nullptr;
	}
}

} // namespace PhysicsUtils

PhysicsWorld::PhysicsWorld()
	: PhysicsWorld(Float3{ 0.0f, -9.81f, 0.0f })
{

}

PhysicsWorld::PhysicsWorld(const Float3& gravity)
{
	b3WorldDef def = b3DefaultWorldDef();
	def.gravity = PhysicsUtils::ToBox3D(gravity);
	mWorldHandle = PhysicsUtils::FromWorldId(b3CreateWorld(&def));
}

PhysicsWorld::~PhysicsWorld()
{
	if (IsValid())
	{
		b3DestroyWorld(PhysicsUtils::ToWorldId(mWorldHandle));
		mWorldHandle = 0;
	}
}

PhysicsWorld::PhysicsWorld(PhysicsWorld&& other) noexcept
	: mWorldHandle(other.mWorldHandle)
{
	other.mWorldHandle = 0;
}

PhysicsWorld& PhysicsWorld::operator=(PhysicsWorld&& other) noexcept
{
	if (this != &other)
	{
		if (IsValid())
		{
			b3DestroyWorld(PhysicsUtils::ToWorldId(mWorldHandle));
		}
		mWorldHandle = other.mWorldHandle;
		other.mWorldHandle = 0;
	}
	return *this;
}

bool PhysicsWorld::IsValid() const
{
	return b3World_IsValid(PhysicsUtils::ToWorldId(mWorldHandle));
}

void PhysicsWorld::Step(float deltaTime)
{
	static constexpr int kSubStepCount = 4;
	b3World_Step(PhysicsUtils::ToWorldId(mWorldHandle), deltaTime, kSubStepCount);
}

void PhysicsWorld::SetGravity(const Float3& gravity)
{
	b3World_SetGravity(PhysicsUtils::ToWorldId(mWorldHandle), PhysicsUtils::ToBox3D(gravity));
}

Float3 PhysicsWorld::GetGravity() const
{
	return PhysicsUtils::FromBox3D(b3World_GetGravity(PhysicsUtils::ToWorldId(mWorldHandle)));
}

RigidBodyHandle PhysicsWorld::CreateRigidBody(const RigidBody& rigidBody, const Float3& position, const Quaternion& rotation, void* userData)
{
	b3BodyDef def = b3DefaultBodyDef();
	def.type = PhysicsUtils::ToBox3D(rigidBody.type);
	def.position = PhysicsUtils::ToBox3D(position);
	def.rotation = PhysicsUtils::ToBox3D(rotation);
	def.linearDamping = rigidBody.linearDamping;
	def.angularDamping = rigidBody.angularDamping;
	def.gravityScale = rigidBody.gravityScale;
	def.enableSleep = rigidBody.enableSleep;
	def.isBullet = rigidBody.isBullet;
	def.userData = userData;
	return PhysicsUtils::FromBox3D(b3CreateBody(PhysicsUtils::ToWorldId(mWorldHandle), &def));
}

void PhysicsWorld::DestroyRigidBody(RigidBodyHandle body)
{
	const b3BodyId id = PhysicsUtils::ToBox3D(body);
	if (b3Body_IsValid(id))
	{
		b3DestroyBody(id);
	}
}

bool PhysicsWorld::IsValidRigidBody(RigidBodyHandle body) const
{
	return b3Body_IsValid(PhysicsUtils::ToBox3D(body));
}

ColliderHandle PhysicsWorld::CreateSphereCollider(RigidBodyHandle body, const SphereCollider& collider, const PhysicsMaterial& material, float density, float scale, void* userData)
{
	const b3ShapeDef def = PhysicsUtils::MakeShapeDef(material, density, collider.isTrigger, userData);
	const b3Sphere geometry = { PhysicsUtils::ToBox3D(collider.center * scale), collider.radius * scale };
	return PhysicsUtils::FromBox3D(b3CreateSphereShape(PhysicsUtils::ToBox3D(body), &def, &geometry));
}

ColliderHandle PhysicsWorld::CreateBoxCollider(RigidBodyHandle body, const BoxCollider& collider, const PhysicsMaterial& material, float density, float scale, void* userData)
{
	const b3ShapeDef def = PhysicsUtils::MakeShapeDef(material, density, collider.isTrigger, userData);
	const Float3 halfExtents = collider.size * (0.5f * scale);
	const b3Transform transform = { PhysicsUtils::ToBox3D(collider.center * scale), PhysicsUtils::ToBox3D(collider.rotation) };
	const b3BoxHull hull = b3MakeTransformedBoxHull(halfExtents.x, halfExtents.y, halfExtents.z, transform);
	return PhysicsUtils::FromBox3D(b3CreateHullShape(PhysicsUtils::ToBox3D(body), &def, &hull.base));
}

ColliderHandle PhysicsWorld::CreateCapsuleCollider(RigidBodyHandle body, const CapsuleCollider& collider, const PhysicsMaterial& material, float density, float scale, void* userData)
{
	const b3ShapeDef def = PhysicsUtils::MakeShapeDef(material, density, collider.isTrigger, userData);
	const Float3 center = collider.center * scale;
	const float halfSegment = Math::Max(0.0f, collider.height * 0.5f - collider.radius) * scale;
	const Float3 axis = collider.rotation * Float3{ 0.0f, halfSegment, 0.0f };
	const b3Capsule geometry = { PhysicsUtils::ToBox3D(center - axis), PhysicsUtils::ToBox3D(center + axis), collider.radius * scale };
	return PhysicsUtils::FromBox3D(b3CreateCapsuleShape(PhysicsUtils::ToBox3D(body), &def, &geometry));
}

void PhysicsWorld::DestroyCollider(ColliderHandle collider)
{
	const b3ShapeId id = PhysicsUtils::ToBox3D(collider);
	if (b3Shape_IsValid(id))
	{
		b3DestroyShape(id, true);
	}
}

void PhysicsWorld::SetRigidBodyType(RigidBodyHandle body, RigidBodyType type)
{
	b3Body_SetType(PhysicsUtils::ToBox3D(body), PhysicsUtils::ToBox3D(type));
}

void PhysicsWorld::SetRigidBodyTransform(RigidBodyHandle body, const Float3& position, const Quaternion& rotation)
{
	b3Body_SetTransform(PhysicsUtils::ToBox3D(body), PhysicsUtils::ToBox3D(position), PhysicsUtils::ToBox3D(rotation));
}

Float3 PhysicsWorld::GetRigidBodyPosition(RigidBodyHandle body) const
{
	return PhysicsUtils::FromBox3D(b3Body_GetPosition(PhysicsUtils::ToBox3D(body)));
}

Quaternion PhysicsWorld::GetRigidBodyRotation(RigidBodyHandle body) const
{
	return PhysicsUtils::FromBox3D(b3Body_GetRotation(PhysicsUtils::ToBox3D(body)));
}

void PhysicsWorld::SetLinearVelocity(RigidBodyHandle body, const Float3& velocity)
{
	b3Body_SetLinearVelocity(PhysicsUtils::ToBox3D(body), PhysicsUtils::ToBox3D(velocity));
}

Float3 PhysicsWorld::GetLinearVelocity(RigidBodyHandle body) const
{
	return PhysicsUtils::FromBox3D(b3Body_GetLinearVelocity(PhysicsUtils::ToBox3D(body)));
}

void PhysicsWorld::SetAngularVelocity(RigidBodyHandle body, const Float3& velocity)
{
	b3Body_SetAngularVelocity(PhysicsUtils::ToBox3D(body), PhysicsUtils::ToBox3D(velocity));
}

Float3 PhysicsWorld::GetAngularVelocity(RigidBodyHandle body) const
{
	return PhysicsUtils::FromBox3D(b3Body_GetAngularVelocity(PhysicsUtils::ToBox3D(body)));
}

void PhysicsWorld::SetLinearDamping(RigidBodyHandle body, float damping)
{
	b3Body_SetLinearDamping(PhysicsUtils::ToBox3D(body), damping);
}

void PhysicsWorld::SetAngularDamping(RigidBodyHandle body, float damping)
{
	b3Body_SetAngularDamping(PhysicsUtils::ToBox3D(body), damping);
}

void PhysicsWorld::SetGravityScale(RigidBodyHandle body, float scale)
{
	b3Body_SetGravityScale(PhysicsUtils::ToBox3D(body), scale);
}

void PhysicsWorld::ApplyForce(RigidBodyHandle body, const Float3& force, bool wake)
{
	b3Body_ApplyForceToCenter(PhysicsUtils::ToBox3D(body), PhysicsUtils::ToBox3D(force), wake);
}

void PhysicsWorld::ApplyForceAtPoint(RigidBodyHandle body, const Float3& force, const Float3& point, bool wake)
{
	b3Body_ApplyForce(PhysicsUtils::ToBox3D(body), PhysicsUtils::ToBox3D(force), PhysicsUtils::ToBox3D(point), wake);
}

void PhysicsWorld::ApplyTorque(RigidBodyHandle body, const Float3& torque, bool wake)
{
	b3Body_ApplyTorque(PhysicsUtils::ToBox3D(body), PhysicsUtils::ToBox3D(torque), wake);
}

void PhysicsWorld::ApplyLinearImpulse(RigidBodyHandle body, const Float3& impulse, bool wake)
{
	b3Body_ApplyLinearImpulseToCenter(PhysicsUtils::ToBox3D(body), PhysicsUtils::ToBox3D(impulse), wake);
}

void PhysicsWorld::ApplyAngularImpulse(RigidBodyHandle body, const Float3& impulse, bool wake)
{
	b3Body_ApplyAngularImpulse(PhysicsUtils::ToBox3D(body), PhysicsUtils::ToBox3D(impulse), wake);
}

void PhysicsWorld::SetRigidBodyAwake(RigidBodyHandle body, bool awake)
{
	b3Body_SetAwake(PhysicsUtils::ToBox3D(body), awake);
}

bool PhysicsWorld::IsRigidBodyAwake(RigidBodyHandle body) const
{
	return b3Body_IsAwake(PhysicsUtils::ToBox3D(body));
}

PhysicsRaycastHit PhysicsWorld::Raycast(const Float3& origin, const Float3& direction, float distance) const
{
	b3RayResult result = b3World_CastRayClosest(PhysicsUtils::ToWorldId(mWorldHandle), PhysicsUtils::ToBox3D(origin), PhysicsUtils::ToBox3D(direction * distance), b3DefaultQueryFilter());

	PhysicsRaycastHit hit;
	if (result.hit)
	{
		hit.point = PhysicsUtils::FromBox3D(result.point);
		hit.normal = PhysicsUtils::FromBox3D(result.normal);
		hit.distance = result.fraction * distance;
		hit.userData = PhysicsUtils::GetShapeBodyUserData(result.shapeId);
		hit.hit = true;
	}
	return hit;
}

void PhysicsWorld::ForEachRigidBodyMotion(MotionFn&& fn) const
{
	const b3BodyEvents events = b3World_GetBodyEvents(PhysicsUtils::ToWorldId(mWorldHandle));
	for (int i = 0; i < events.moveCount; ++i)
	{
		const b3BodyMoveEvent& event = events.moveEvents[i];

		RigidBodyMotion motion;
		motion.position = PhysicsUtils::FromBox3D(event.transform.p);
		motion.rotation = PhysicsUtils::FromBox3D(event.transform.q);
		motion.userData = event.userData;
		motion.fellAsleep = event.fellAsleep;
		fn(motion);
	}
}

void PhysicsWorld::ForEachContactBegin(ContactFn&& fn) const
{
	const b3ContactEvents events = b3World_GetContactEvents(PhysicsUtils::ToWorldId(mWorldHandle));
	for (int i = 0; i < events.beginCount; ++i)
	{
		const b3ContactBeginTouchEvent& event = events.beginEvents[i];

		PhysicsContact contact;
		contact.userDataA = PhysicsUtils::GetShapeBodyUserData(event.shapeIdA);
		contact.userDataB = PhysicsUtils::GetShapeBodyUserData(event.shapeIdB);
		fn(contact);
	}
}

void PhysicsWorld::ForEachContactEnd(ContactFn&& fn) const
{
	const b3ContactEvents events = b3World_GetContactEvents(PhysicsUtils::ToWorldId(mWorldHandle));
	for (int i = 0; i < events.endCount; ++i)
	{
		const b3ContactEndTouchEvent& event = events.endEvents[i];

		PhysicsContact contact;
		contact.userDataA = PhysicsUtils::GetShapeBodyUserData(event.shapeIdA);
		contact.userDataB = PhysicsUtils::GetShapeBodyUserData(event.shapeIdB);
		fn(contact);
	}
}
