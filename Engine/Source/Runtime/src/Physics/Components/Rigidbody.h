#pragma once
#include <Reflection/Macro.h>
#include "Physics/PhysicsTypes.h"

namespace Gleam {

GSTRUCT(Rigidbody, "3E1F9344-7EE1-44D6-AE82-79A8A11B88A6", EntityComponent, Serializable)
{
	GFIELD("D8E9809A-8E21-4D69-9FE2-8A8192A37D1F", Serializable, PrettyName("Body Type"))
	PhysicsBodyType type = PhysicsBodyType::Dynamic;

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
};

} // namespace Gleam
