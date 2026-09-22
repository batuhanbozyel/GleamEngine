#pragma once
#include <Reflection/Macro.h>
#include "Math/Vector3.h"
#include "Math/Quaternion.h"
#include "Container/Array.h"
#include "Assets/AssetReference.h"

namespace Gleam {

GSTRUCT(BoxCollider, "9C4A0E56-1B18-4E3F-B1F6-04D4707990D5", Serializable)
{
	GFIELD("654E98B7-C201-49C5-BC0F-51FDED825BC9", Serializable, PrettyName("Center"))
	Float3 center = Float3::zero;

	GFIELD("A01B9B4C-ADAE-4466-8B40-CBDCA0FA9B2C", Serializable, PrettyName("Rotation"))
	Quaternion rotation = Quaternion::identity;

	GFIELD("792CC823-89F3-4F83-AFB1-88D440218095", Serializable, PrettyName("Size"))
	Float3 size = Float3::one;

	GFIELD("58FF2578-6C08-4A34-8C8A-CA04D3E23A6A", Serializable, PrettyName("Is Trigger"))
	bool isTrigger = false;
};

GSTRUCT(SphereCollider, "01398EF2-1896-4CC1-9139-EB389A3BE257", Serializable)
{
	GFIELD("3A0A06C1-C467-48FC-B289-62201AB784D1", Serializable, PrettyName("Center"))
	Float3 center = Float3::zero;

	GFIELD("9431634E-FED3-427D-8EA8-A14DF5EF62FD", Serializable, PrettyName("Radius"))
	float radius = 0.5f;

	GFIELD("88E79F02-499E-437C-9BFD-65A1C426D59C", Serializable, PrettyName("Is Trigger"))
	bool isTrigger = false;
};

GSTRUCT(CapsuleCollider, "221FEA00-DCED-47E1-8779-BD73D4596EA5", Serializable)
{
	GFIELD("67D239B4-5AAD-4D14-8287-2262D4209659", Serializable, PrettyName("Center"))
	Float3 center = Float3::zero;

	GFIELD("9668CBB0-C610-40C9-98D0-7B15C67C781A", Serializable, PrettyName("Rotation"))
	Quaternion rotation = Quaternion::identity;

	GFIELD("8795C51E-2C4E-451D-BB11-8BC809815653", Serializable, PrettyName("Radius"))
	float radius = 0.5f;

	GFIELD("FCC7A722-D8D2-48CD-8E06-5758C3DB14FD", Serializable, PrettyName("Height"))
	float height = 2.0f;

	GFIELD("637F2199-F988-4982-B62D-C389F23AFAC2", Serializable, PrettyName("Is Trigger"))
	bool isTrigger = false;
};

GSTRUCT(ConvexMeshCollider, "B35ED357-460B-4D7A-BAAC-E6E9CE263E4E", Serializable)
{
	GFIELD("1FECB525-7A03-4946-94C3-18E10E17FF92", Serializable, PrettyName("Mesh"))
	AssetReference mesh;

	GFIELD("DBD647AE-E0A3-4860-A265-668571B25CE1", Serializable, PrettyName("Center"))
	Float3 center = Float3::zero;

	GFIELD("120CBE74-C5EF-4145-9844-2B529F684DFD", Serializable, PrettyName("Rotation"))
	Quaternion rotation = Quaternion::identity;

	GFIELD("19E48FA6-0783-4958-A8FE-DEEA0A93DE29", Serializable, PrettyName("Is Trigger"))
	bool isTrigger = false;
};

GSTRUCT(TriangleMeshCollider, "D4D01218-F598-451E-A981-227075C7696A", Serializable)
{
	GFIELD("DB562E22-BEC1-4077-99A0-83E4EDC209DA", Serializable, PrettyName("Mesh"))
	AssetReference mesh;

	GFIELD("BA4506BB-A9E3-4789-8C88-0D1D05766397", Serializable, PrettyName("Is Trigger"))
	bool isTrigger = false;
};

GSTRUCT(ColliderSet, "7EBECB88-4AD2-433C-AC62-2A286116D5FA", Serializable)
{
	GFIELD("A0FBF183-A489-477A-AAEA-0F170922C13B", Serializable, PrettyName("Boxes"))
	TArray<BoxCollider> boxes;

	GFIELD("4839A3A1-51C2-4F84-B0CA-6349E3E4F1C3", Serializable, PrettyName("Spheres"))
	TArray<SphereCollider> spheres;

	GFIELD("5F14F2DA-024F-43CB-9B63-C742F80A1B8B", Serializable, PrettyName("Capsules"))
	TArray<CapsuleCollider> capsules;

	GFIELD("3738CA41-89BA-4366-8D8D-56867690F55A", Serializable, PrettyName("Convex Meshes"))
	TArray<ConvexMeshCollider> convexMeshes;

	GFIELD("1152DE2B-5574-4A77-8866-B660B879759D", Serializable, PrettyName("Triangle Meshes"))
	TArray<TriangleMeshCollider> triangleMeshes;

	NO_DISCARD FORCE_INLINE bool Empty() const
	{
		return boxes.empty() and spheres.empty() and capsules.empty() and convexMeshes.empty() and triangleMeshes.empty();
	}
};

} // namespace Gleam
