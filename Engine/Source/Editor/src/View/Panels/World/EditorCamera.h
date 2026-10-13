#pragma once
#include "Core/Attributes.h"
#include "Math/Vector3.h"

namespace Gleam {

GSTRUCT(EditorCamera, "17B11CC4-B5D0-40C8-BEF6-0D3825C2DC4F", EntityComponent, EditorOnly)
{
	float yaw = 0.0f;
	float pitch = 0.0f;
	float distance = 10.0f;
	float speed = 5.0f;
};

GSTRUCT(EditorCameraState, "85C7BC60-DA00-488D-A8F2-BF51E38D3C6F", Serializable)
{
	GFIELD("6E4E8CF6-0FD2-4586-AE39-B814AC34B768", Serializable)
	Float3 position = Float3::zero;

	GFIELD("3835927F-E48B-47E0-AC0A-9907B6D5AE90", Serializable)
	float yaw = 0.0f;

	GFIELD("BE297C15-FB66-4DCE-9418-06F1DF4BD714", Serializable)
	float pitch = 0.0f;

	GFIELD("694621FA-73B7-41A4-9654-E22F0FE2BF3C", Serializable)
	float distance = 10.0f;

	GFIELD("092D2CC2-E987-409C-94D7-6F02DCB90A32", Serializable)
	float speed = 5.0f;
};

} // namespace Gleam
