#pragma once
#include <Reflection/Macro.h>
#include "Physics/PhysicsTypes.h"

namespace Gleam {

GSTRUCT(BoxCollider, "9C4A0E56-1B18-4E3F-B1F6-04D4707990D5", EntityComponent, Serializable)
{
	GFIELD("654E98B7-C201-49C5-BC0F-51FDED825BC9", Serializable, PrettyName("Center"))
	Float3 center = Float3::zero;

	GFIELD("792CC823-89F3-4F83-AFB1-88D440218095", Serializable, PrettyName("Size"))
	Float3 size = Float3::one;

	GFIELD("714B40F0-9FDD-44EC-BCD4-19D64552C53B", Serializable, PrettyName("Material"))
	PhysicsMaterial material = {};

	GFIELD("58FF2578-6C08-4A34-8C8A-CA04D3E23A6A", Serializable, PrettyName("Is Trigger"))
	bool isTrigger = false;
};

GSTRUCT(SphereCollider, "01398EF2-1896-4CC1-9139-EB389A3BE257", EntityComponent, Serializable)
{
	GFIELD("3A0A06C1-C467-48FC-B289-62201AB784D1", Serializable, PrettyName("Center"))
	Float3 center = Float3::zero;

	GFIELD("9431634E-FED3-427D-8EA8-A14DF5EF62FD", Serializable, PrettyName("Radius"))
	float radius = 0.5f;

	GFIELD("D37F8836-CE02-459F-AB7D-C7DBBC44C266", Serializable, PrettyName("Material"))
	PhysicsMaterial material = {};

	GFIELD("88E79F02-499E-437C-9BFD-65A1C426D59C", Serializable, PrettyName("Is Trigger"))
	bool isTrigger = false;
};

GSTRUCT(CapsuleCollider, "221FEA00-DCED-47E1-8779-BD73D4596EA5", EntityComponent, Serializable)
{
	GFIELD("67D239B4-5AAD-4D14-8287-2262D4209659", Serializable, PrettyName("Center"))
	Float3 center = Float3::zero;

	GFIELD("8795C51E-2C4E-451D-BB11-8BC809815653", Serializable, PrettyName("Radius"))
	float radius = 0.5f;

	GFIELD("FCC7A722-D8D2-48CD-8E06-5758C3DB14FD", Serializable, PrettyName("Height"))
	float height = 2.0f;

	GFIELD("C8F802D0-D6D5-4634-8467-720C63463966", Serializable, PrettyName("Material"))
	PhysicsMaterial material = {};

	GFIELD("637F2199-F988-4982-B62D-C389F23AFAC2", Serializable, PrettyName("Is Trigger"))
	bool isTrigger = false;
};

} // namespace Gleam
