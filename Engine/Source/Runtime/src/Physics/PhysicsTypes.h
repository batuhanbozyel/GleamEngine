#pragma once
#include <Reflection/Macro.h>
#include "Math/Vector3.h"
#include "Math/Quaternion.h"

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

struct PhysicsRaycastHit
{
	Float3 point = Float3::zero;
	Float3 normal = Float3::zero;
	float distance = 0.0f;
	void* userData = nullptr;
	bool hit = false;
};

struct RigidBodyMotion
{
	Float3 position = Float3::zero;
	Quaternion rotation = Quaternion::identity;
	void* userData = nullptr;
	bool fellAsleep = false;
};

struct PhysicsContact
{
	void* userDataA = nullptr;
	void* userDataB = nullptr;
};

} // namespace Gleam
