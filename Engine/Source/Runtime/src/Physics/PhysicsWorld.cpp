#include "gpch.h"
#include "PhysicsWorld.h"

#include <box3d/box3d.h>
#include <cstring>

using namespace Gleam;

namespace PhysicsUtils {

static_assert(sizeof(b3WorldId) <= sizeof(uint64_t), "b3WorldId does not fit in a PhysicsWorld handle.");
static_assert(sizeof(b3BodyId) <= sizeof(uint64_t), "b3BodyId does not fit in a PhysicsBodyHandle.");
static_assert(sizeof(b3ShapeId) <= sizeof(uint64_t), "b3ShapeId does not fit in a PhysicsShapeHandle.");

b3WorldId ToWorldId(uint64_t handle)
{
	b3WorldId id = {};
	std::memcpy(&id, &handle, sizeof(id));
	return id;
}

uint64_t FromWorldId(b3WorldId id)
{
	uint64_t handle = 0;
	std::memcpy(&handle, &id, sizeof(id));
	return handle;
}

b3BodyId ToBox3D(PhysicsBodyHandle handle)
{
	b3BodyId id = {};
	std::memcpy(&id, &handle.value, sizeof(id));
	return id;
}

PhysicsBodyHandle FromBox3D(b3BodyId id)
{
	PhysicsBodyHandle handle;
	std::memcpy(&handle.value, &id, sizeof(id));
	return handle;
}

b3ShapeId ToBox3D(PhysicsShapeHandle handle)
{
	b3ShapeId id = {};
	std::memcpy(&id, &handle.value, sizeof(id));
	return id;
}

PhysicsShapeHandle FromBox3D(b3ShapeId id)
{
	PhysicsShapeHandle handle;
	std::memcpy(&handle.value, &id, sizeof(id));
	return handle;
}

b3Vec3 ToBox3D(const Float3& vec)
{
	return b3Vec3{ vec.x, vec.y, vec.z };
}

Float3 FromBox3D(const b3Vec3& vec)
{
	return Float3{ vec.x, vec.y, vec.z };
}

b3Quat ToBox3D(const Quaternion& quat)
{
	return b3Quat{ { quat.x, quat.y, quat.z }, quat.w };
}

Quaternion FromBox3D(const b3Quat& quat)
{
	return Quaternion{ quat.s, quat.v.x, quat.v.y, quat.v.z };
}

b3BodyType ToBox3D(PhysicsBodyType type)
{
	if (type == PhysicsBodyType::Static)
	{
		return b3_staticBody;
	}
	else if (type == PhysicsBodyType::Kinematic)
	{
		return b3_kinematicBody;
	}
	else
	{
		return b3_dynamicBody;
	}
}

b3ShapeDef ToBox3D(const PhysicsShapeDescriptor& descriptor)
{
	b3ShapeDef def = b3DefaultShapeDef();
	def.density = descriptor.material.density;
	def.baseMaterial.friction = descriptor.material.friction;
	def.baseMaterial.restitution = descriptor.material.restitution;
	def.baseMaterial.rollingResistance = descriptor.material.rollingResistance;
	def.isSensor = descriptor.isSensor;
	def.enableSensorEvents = descriptor.isSensor;
	def.enableContactEvents = descriptor.enableContactEvents;
	def.userData = descriptor.userData;
	return def;
}

void* GetShapeBodyUserData(b3ShapeId shape)
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

void PhysicsWorld::Step(float deltaTime, uint32_t subStepCount)
{
	b3World_Step(PhysicsUtils::ToWorldId(mWorldHandle), deltaTime, static_cast<int>(subStepCount));
}

void PhysicsWorld::SetGravity(const Float3& gravity)
{
	b3World_SetGravity(PhysicsUtils::ToWorldId(mWorldHandle), PhysicsUtils::ToBox3D(gravity));
}

Float3 PhysicsWorld::GetGravity() const
{
	return PhysicsUtils::FromBox3D(b3World_GetGravity(PhysicsUtils::ToWorldId(mWorldHandle)));
}

PhysicsBodyHandle PhysicsWorld::CreateBody(const PhysicsBodyDescriptor& descriptor)
{
	b3BodyDef def = b3DefaultBodyDef();
	def.type = PhysicsUtils::ToBox3D(descriptor.type);
	def.position = PhysicsUtils::ToBox3D(descriptor.position);
	def.rotation = PhysicsUtils::ToBox3D(descriptor.rotation);
	def.linearVelocity = PhysicsUtils::ToBox3D(descriptor.linearVelocity);
	def.angularVelocity = PhysicsUtils::ToBox3D(descriptor.angularVelocity);
	def.linearDamping = descriptor.linearDamping;
	def.angularDamping = descriptor.angularDamping;
	def.gravityScale = descriptor.gravityScale;
	def.enableSleep = descriptor.enableSleep;
	def.isBullet = descriptor.isBullet;
	def.userData = descriptor.userData;
	return PhysicsUtils::FromBox3D(b3CreateBody(PhysicsUtils::ToWorldId(mWorldHandle), &def));
}

void PhysicsWorld::DestroyBody(PhysicsBodyHandle body)
{
	const b3BodyId id = PhysicsUtils::ToBox3D(body);
	if (b3Body_IsValid(id))
	{
		b3DestroyBody(id);
	}
}

bool PhysicsWorld::IsValidBody(PhysicsBodyHandle body) const
{
	return b3Body_IsValid(PhysicsUtils::ToBox3D(body));
}

PhysicsShapeHandle PhysicsWorld::CreateSphereShape(PhysicsBodyHandle body, const PhysicsShapeDescriptor& descriptor, const PhysicsSphereShape& sphere)
{
	const b3ShapeDef def = PhysicsUtils::ToBox3D(descriptor);
	const b3Sphere geometry = { PhysicsUtils::ToBox3D(sphere.center), sphere.radius };
	return PhysicsUtils::FromBox3D(b3CreateSphereShape(PhysicsUtils::ToBox3D(body), &def, &geometry));
}

PhysicsShapeHandle PhysicsWorld::CreateBoxShape(PhysicsBodyHandle body, const PhysicsShapeDescriptor& descriptor, const PhysicsBoxShape& box)
{
	const b3ShapeDef def = PhysicsUtils::ToBox3D(descriptor);
	const b3BoxHull hull = b3MakeOffsetBoxHull(box.halfExtents.x, box.halfExtents.y, box.halfExtents.z, PhysicsUtils::ToBox3D(box.center));
	return PhysicsUtils::FromBox3D(b3CreateHullShape(PhysicsUtils::ToBox3D(body), &def, &hull.base));
}

PhysicsShapeHandle PhysicsWorld::CreateCapsuleShape(PhysicsBodyHandle body, const PhysicsShapeDescriptor& descriptor, const PhysicsCapsuleShape& capsule)
{
	const b3ShapeDef def = PhysicsUtils::ToBox3D(descriptor);
	const Float3 axis = Float3{ 0.0f, capsule.halfHeight, 0.0f };
	const b3Capsule geometry = { PhysicsUtils::ToBox3D(capsule.center - axis), PhysicsUtils::ToBox3D(capsule.center + axis), capsule.radius };
	return PhysicsUtils::FromBox3D(b3CreateCapsuleShape(PhysicsUtils::ToBox3D(body), &def, &geometry));
}

void PhysicsWorld::DestroyShape(PhysicsShapeHandle shape)
{
	const b3ShapeId id = PhysicsUtils::ToBox3D(shape);
	if (b3Shape_IsValid(id))
	{
		b3DestroyShape(id, true);
	}
}

void PhysicsWorld::SetBodyType(PhysicsBodyHandle body, PhysicsBodyType type)
{
	b3Body_SetType(PhysicsUtils::ToBox3D(body), PhysicsUtils::ToBox3D(type));
}

void PhysicsWorld::SetBodyTransform(PhysicsBodyHandle body, const Float3& position, const Quaternion& rotation)
{
	b3Body_SetTransform(PhysicsUtils::ToBox3D(body), PhysicsUtils::ToBox3D(position), PhysicsUtils::ToBox3D(rotation));
}

Float3 PhysicsWorld::GetBodyPosition(PhysicsBodyHandle body) const
{
	return PhysicsUtils::FromBox3D(b3Body_GetPosition(PhysicsUtils::ToBox3D(body)));
}

Quaternion PhysicsWorld::GetBodyRotation(PhysicsBodyHandle body) const
{
	return PhysicsUtils::FromBox3D(b3Body_GetRotation(PhysicsUtils::ToBox3D(body)));
}

void PhysicsWorld::SetLinearVelocity(PhysicsBodyHandle body, const Float3& velocity)
{
	b3Body_SetLinearVelocity(PhysicsUtils::ToBox3D(body), PhysicsUtils::ToBox3D(velocity));
}

Float3 PhysicsWorld::GetLinearVelocity(PhysicsBodyHandle body) const
{
	return PhysicsUtils::FromBox3D(b3Body_GetLinearVelocity(PhysicsUtils::ToBox3D(body)));
}

void PhysicsWorld::SetAngularVelocity(PhysicsBodyHandle body, const Float3& velocity)
{
	b3Body_SetAngularVelocity(PhysicsUtils::ToBox3D(body), PhysicsUtils::ToBox3D(velocity));
}

Float3 PhysicsWorld::GetAngularVelocity(PhysicsBodyHandle body) const
{
	return PhysicsUtils::FromBox3D(b3Body_GetAngularVelocity(PhysicsUtils::ToBox3D(body)));
}

void PhysicsWorld::SetLinearDamping(PhysicsBodyHandle body, float damping)
{
	b3Body_SetLinearDamping(PhysicsUtils::ToBox3D(body), damping);
}

void PhysicsWorld::SetAngularDamping(PhysicsBodyHandle body, float damping)
{
	b3Body_SetAngularDamping(PhysicsUtils::ToBox3D(body), damping);
}

void PhysicsWorld::SetGravityScale(PhysicsBodyHandle body, float scale)
{
	b3Body_SetGravityScale(PhysicsUtils::ToBox3D(body), scale);
}

void PhysicsWorld::ApplyForce(PhysicsBodyHandle body, const Float3& force, bool wake)
{
	b3Body_ApplyForceToCenter(PhysicsUtils::ToBox3D(body), PhysicsUtils::ToBox3D(force), wake);
}

void PhysicsWorld::ApplyForceAtPoint(PhysicsBodyHandle body, const Float3& force, const Float3& point, bool wake)
{
	b3Body_ApplyForce(PhysicsUtils::ToBox3D(body), PhysicsUtils::ToBox3D(force), PhysicsUtils::ToBox3D(point), wake);
}

void PhysicsWorld::ApplyTorque(PhysicsBodyHandle body, const Float3& torque, bool wake)
{
	b3Body_ApplyTorque(PhysicsUtils::ToBox3D(body), PhysicsUtils::ToBox3D(torque), wake);
}

void PhysicsWorld::ApplyLinearImpulse(PhysicsBodyHandle body, const Float3& impulse, bool wake)
{
	b3Body_ApplyLinearImpulseToCenter(PhysicsUtils::ToBox3D(body), PhysicsUtils::ToBox3D(impulse), wake);
}

void PhysicsWorld::ApplyAngularImpulse(PhysicsBodyHandle body, const Float3& impulse, bool wake)
{
	b3Body_ApplyAngularImpulse(PhysicsUtils::ToBox3D(body), PhysicsUtils::ToBox3D(impulse), wake);
}

void PhysicsWorld::SetBodyAwake(PhysicsBodyHandle body, bool awake)
{
	b3Body_SetAwake(PhysicsUtils::ToBox3D(body), awake);
}

bool PhysicsWorld::IsBodyAwake(PhysicsBodyHandle body) const
{
	return b3Body_IsAwake(PhysicsUtils::ToBox3D(body));
}

PhysicsRaycastHit PhysicsWorld::Raycast(const Float3& origin, const Float3& direction, float distance) const
{
	const b3RayResult result = b3World_CastRayClosest(PhysicsUtils::ToWorldId(mWorldHandle), PhysicsUtils::ToBox3D(origin), PhysicsUtils::ToBox3D(direction * distance), b3DefaultQueryFilter());

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

void PhysicsWorld::ForEachBodyMotion(MotionFn&& fn) const
{
	const b3BodyEvents events = b3World_GetBodyEvents(PhysicsUtils::ToWorldId(mWorldHandle));
	for (int i = 0; i < events.moveCount; ++i)
	{
		const b3BodyMoveEvent& event = events.moveEvents[i];

		PhysicsBodyMotion motion;
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
