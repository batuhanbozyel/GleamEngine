#pragma once
#include "Core/GUID.h"
#include "Reflection/Macro.h"

namespace Gleam {

GSTRUCT(InterleavedMeshVertex, "4AFE936A-550F-419C-A7F0-5ED38D9D1642", Serializable)
{
	GFIELD("6A5DB3C2-9E47-4F31-B82D-47C8E19A5D6F", Serializable)
	Float3 normal;

	GFIELD("8C72F4E1-D5B9-4A63-93C7-E5D64F2B8A19", Serializable)
	Float3 tangent;

	GFIELD("3B9D8E27-C6A5-4F91-B3D2-8A7C6F5E4D3B", Serializable)
	Float2 texCoord;
};

GSTRUCT(SubmeshDescriptor, "DD7E3A74-ADF4-45A9-8DFD-CA252EDC49A6", Serializable)
{
	GFIELD("E2D1C9B8-A7F6-4E5D-B3C2-A1F0E9D8C7B6", Serializable)
	BoundingBox bounds;

	GFIELD("7B6A5D4C-3E2F-41B0-A9D8-C7B6A5D4C3E2", Serializable)
	uint32_t baseVertex = 0;

	GFIELD("F1E2D3C4-B5A6-47B8-91C2-D3E4F5A6B7C8", Serializable)
	uint32_t firstIndex = 0;

	GFIELD("C8B7A6F5-E4D3-42C1-B0A9-F8E7D6C5B4A3", Serializable)
	uint32_t indexCount = 0;

	GFIELD("9A8B7C6D-5E4F-43D2-C1B0-A9F8E7D6C5B4", Serializable)
	uint32_t materialIndex = 0;
};

GSTRUCT(MeshDescriptor, "59E4007E-F7D4-4107-A05F-E1121067DCD3", Serializable)
{
	GFIELD("B1A2C3D4-E5F6-47A8-B9C0-D1E2F3A4B5C6", Serializable)
	TString name;

	GFIELD("D4C3B2A1-F5E6-48B9-C0A9-F3E2D1C0B9A8", Serializable)
	TArray<uint32_t> indices;

	GFIELD("7F8E9D0C-1B2A-49C8-D7E6-5F4E3D2C1B0A", Serializable)
	TArray<Float3> positions;

	GFIELD("A9B8C7D6-E5F4-4A3B-2C1D-0E9F8A7B6C5D", Serializable)
	TArray<InterleavedMeshVertex> interleavedVertices;

	GFIELD("C5D4E3F2-A1B0-4B9C-8D7E-6F5A4B3C2D1E", Serializable)
	TArray<SubmeshDescriptor> submeshes;
};

} // namespace Gleam
