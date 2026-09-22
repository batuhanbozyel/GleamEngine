#pragma once
#include <Reflection/Macro.h>
#include "Physics/Collider.h"

namespace Gleam {

GENUM(RigidBodyType, "C15B6B8F-1D1D-41CB-98CD-FF6A7E203A68", Serializable)
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
};

GSTRUCT(RigidBody, "3E1F9344-7EE1-44D6-AE82-79A8A11B88A6", EntityComponent, Serializable)
{
	GFIELD("D8E9809A-8E21-4D69-9FE2-8A8192A37D1F", Serializable, PrettyName("Body Type"))
	RigidBodyType type = RigidBodyType::Dynamic;

	GFIELD("B5FAA22E-1A74-461C-B8AB-C03657AD3935", Serializable, PrettyName("Mass"))
	float mass = 1.0f;

	GFIELD("FBFAE22E-8B1A-41C1-97A5-8890D2DDACCC", Serializable, PrettyName("Linear Damping"))
	float linearDamping = 0.0f;

	GFIELD("72011494-CF6C-49D2-85BC-145C8D90DFF3", Serializable, PrettyName("Angular Damping"))
	float angularDamping = 0.05f;

	GFIELD("D0D4EBFD-F547-4417-B61A-BDEBD3FCFE83", Serializable, PrettyName("Gravity Scale"))
	float gravityScale = 1.0f;

	GFIELD("F84B40A0-37D6-42E4-8229-21AC45AED565", Serializable, PrettyName("Enable Sleep"))
	bool enableSleep = true;

	GFIELD("478FFED0-B79D-4750-91C8-AB4AFA05BFBB", Serializable, PrettyName("Is Bullet"))
	bool isBullet = false;

	GFIELD("7216D92F-4824-48AD-B1C7-ABD88FCF80CA", Serializable, PrettyName("Material"))
	PhysicsMaterial material = {};

	GFIELD("C2EBFF75-2598-428F-BB2B-01A8872945B1", Serializable, PrettyName("Colliders"))
	ColliderSet colliders;
};

} // namespace Gleam
