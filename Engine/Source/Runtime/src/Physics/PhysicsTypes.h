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

struct PhysicsBodyTag;
struct PhysicsShapeTag;

using PhysicsBodyHandle = PhysicsHandle<PhysicsBodyTag>;
using PhysicsShapeHandle = PhysicsHandle<PhysicsShapeTag>;

GENUM(PhysicsBodyType, "C15B6B8F-1D1D-41CB-98CD-FF6A7E203A68", Serializable)
{
	GITEM(Static, "39252783-54B8-41F9-A1C7-391CD527F973"),
	GITEM(Kinematic, "E244169E-5BAD-4672-841A-DD085F921E09"),
	GITEM(Dynamic, "9BAE2BB2-9EFB-440F-B330-D8E567E96B74")
};

GSTRUCT(PhysicsMaterial, "96C19B4F-6803-420D-A51F-50905F6DEB95", Serializable)
{
	GFIELD("C33B0248-AA32-40B9-98EE-5E3858E17B51", Serializable, PrettyName("Friction"))
	float friction = 0.6f;

	GFIELD("F707034C-6350-4389-99F0-7905B82AC1D3", Serializable, PrettyName("Restitution"))
	float restitution = 0.0f;

	GFIELD("565D6933-5A0E-4F79-A6D7-650D413F81D2", Serializable, PrettyName("Rolling Resistance"))
	float rollingResistance = 0.0f;

	GFIELD("50EB1193-964F-4455-8326-0365294DD496", Serializable, PrettyName("Density"))
	float density = 1000.0f;
};

struct PhysicsBodyDescriptor
{
	Float3 position = Float3::zero;
	Quaternion rotation = Quaternion::identity;
	Float3 linearVelocity = Float3::zero;
	Float3 angularVelocity = Float3::zero;
	PhysicsBodyType type = PhysicsBodyType::Dynamic;
	float linearDamping = 0.0f;
	float angularDamping = 0.05f;
	float gravityScale = 1.0f;
	bool enableSleep = true;
	bool isBullet = false;
	void* userData = nullptr;
};

struct PhysicsShapeDescriptor
{
	PhysicsMaterial material = {};
	bool isSensor = false;
	bool enableContactEvents = true;
	void* userData = nullptr;
};

struct PhysicsSphereShape
{
	Float3 center = Float3::zero;
	float radius = 0.5f;
};

struct PhysicsBoxShape
{
	Float3 center = Float3::zero;
	Float3 halfExtents = Float3(0.5f);
};

struct PhysicsCapsuleShape
{
	Float3 center = Float3::zero;
	float radius = 0.5f;
	float halfHeight = 0.5f;
};

struct PhysicsRaycastHit
{
	Float3 point = Float3::zero;
	Float3 normal = Float3::zero;
	float distance = 0.0f;
	void* userData = nullptr;
	bool hit = false;
};

struct PhysicsBodyMotion
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
