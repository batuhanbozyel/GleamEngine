#pragma once
#include "Core/Attributes.h"
#include "Math/Vector3.h"
#include "Math/Quaternion.h"
#include "World/Entity.h"

namespace Gleam {

using NativePhysicsHandle = uint64_t;

template<typename Tag>
struct PhysicsHandle
{
	NativePhysicsHandle value = 0;

	NO_DISCARD FORCE_INLINE constexpr bool IsValid() const
	{
		return value != 0;
	}

	NO_DISCARD FORCE_INLINE constexpr bool operator==(const PhysicsHandle& rhs) const
	{
		return value == rhs.value;
	}

	NO_DISCARD FORCE_INLINE constexpr bool operator!=(const PhysicsHandle& rhs) const
	{
		return not (*this == rhs);
	}
};

struct RigidBodyTag;
struct ColliderTag;

using RigidBodyHandle = PhysicsHandle<RigidBodyTag>;
using ColliderHandle = PhysicsHandle<ColliderTag>;

struct RigidBodyPose
{
	Float3 position = Float3::zero;
	Quaternion rotation = Quaternion::identity;

	constexpr RigidBodyPose() = default;

	constexpr RigidBodyPose(const Float3& position, const Quaternion& rotation)
		: position(position), rotation(rotation)
	{

	}

	constexpr RigidBodyPose(const Transform& transform)
		: position(transform.position), rotation(transform.rotation)
	{

	}

	NO_DISCARD FORCE_INLINE constexpr bool operator==(const RigidBodyPose& rhs) const
	{
		return position == rhs.position and rotation == rhs.rotation;
	}

	NO_DISCARD FORCE_INLINE constexpr bool operator!=(const RigidBodyPose& rhs) const
	{
		return not (*this == rhs);
	}
};

struct PhysicsRaycastHit
{
	EntityHandle entity = InvalidEntity;
	Float3 point = Float3::zero;
	Float3 normal = Float3::zero;
	float distance = 0.0f;
	bool hit = false;
};

struct RigidBodyMotion
{
	EntityHandle entity = InvalidEntity;
	RigidBodyPose pose = {};
};

struct PhysicsContact
{
	EntityHandle entityA = InvalidEntity;
	EntityHandle entityB = InvalidEntity;
};

namespace Math {

NO_DISCARD FORCE_INLINE constexpr RigidBodyPose Lerp(const RigidBodyPose& p0, const RigidBodyPose& p1, float t)
{
	return RigidBodyPose{ Lerp(p0.position, p1.position, t), Nlerp(p0.rotation, p1.rotation, t) };
}

} // namespace Math

} // namespace Gleam
