#pragma once
#include "Math/Vector2.h"
#include "Math/Vector3.h"
#include "Math/Vector4.h"
#include "Math/BoundingBox.h"
#include "Container/String.h"
#include "Container/Array.h"

namespace Gleam {

GSTRUCT(InterleavedMeshVertex, "4AFE936A-550F-419C-A7F0-5ED38D9D1642", Serializable)
{
	GFIELD("6A5DB3C2-9E47-4F31-B82D-47C8E19A5D6F", Serializable)
	Float3 normal;

	GFIELD("8C72F4E1-D5B9-4A63-93C7-E5D64F2B8A19", Serializable)
	Float4 tangent;

	GFIELD("3B9D8E27-C6A5-4F91-B3D2-8A7C6F5E4D3B", Serializable)
	Float2 texCoord;
};

GSTRUCT(MeshletDescriptor, "D3C237CF-8AEB-4735-A890-FCCFE51D69A9", Serializable)
{
	GFIELD("4D2C1B0A-9F8E-47D6-B5C4-A3F2E1D0C9B8", Serializable)
	Float3 center;

	GFIELD("1A0B9C8D-7E6F-45A4-B3C2-D1E0F9A8B7C6", Serializable)
	float radius = 0.0f;

	GFIELD("8E7D6C5B-4A3F-42E1-90D8-C7B6A5F4E3D2", Serializable)
	Float3 coneApex;

	GFIELD("3F2E1D0C-9B8A-4736-A5C4-B3D2E1F0A9B8", Serializable)
	Float3 coneAxis;

	GFIELD("B7A6F5E4-D3C2-41B0-A9F8-E7D6C5B4A3F2", Serializable)
	float coneCutoff = 0.0f;

	GFIELD("6C5B4A3F-2E1D-40C9-B8A7-F6E5D4C3B2A1", Serializable)
	uint32_t vertexOffset = 0;

	GFIELD("D0C9B8A7-F6E5-44D3-A2B1-C0F9E8D7C6B5", Serializable)
	uint32_t triangleOffset = 0;

	GFIELD("2B1A0F9E-8D7C-4B6A-95F4-E3D2C1B0A9F8", Serializable)
	uint32_t vertexCount = 0;

	GFIELD("9F8E7D6C-5B4A-43F2-A1D0-C9B8A7F6E5D4", Serializable)
	uint32_t triangleCount = 0;
};

GSTRUCT(SubmeshDescriptor, "DD7E3A74-ADF4-45A9-8DFD-CA252EDC49A6", Serializable)
{
	GFIELD("E2D1C9B8-A7F6-4E5D-B3C2-A1F0E9D8C7B6", Serializable)
	BoundingBox bounds;

	GFIELD("7B6A5D4C-3E2F-41B0-A9D8-C7B6A5D4C3E2", Serializable)
	uint32_t baseVertex = 0;

	GFIELD("2EDAD211-496A-4A02-BFD7-00B1195EBBA0", Serializable)
	uint32_t vertexCount = 0;

	GFIELD("F1E2D3C4-B5A6-47B8-91C2-D3E4F5A6B7C8", Serializable)
	uint32_t firstIndex = 0;

	GFIELD("C8B7A6F5-E4D3-42C1-B0A9-F8E7D6C5B4A3", Serializable)
	uint32_t indexCount = 0;

	GFIELD("9A8B7C6D-5E4F-43D2-C1B0-A9F8E7D6C5B4", Serializable)
	uint32_t materialIndex = 0;

	GFIELD("1851E003-3E18-42EF-A431-419BD530C3CC", Serializable)
	TArray<MeshletDescriptor> meshlets;
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

	GFIELD("66E8168A-56E4-43FF-BF76-DD5DD3F1AD9B", Serializable)
	TArray<uint32_t> meshletVertices;

	GFIELD("A35FA938-983A-4903-99BA-C0C2F0800054", Serializable)
	TArray<uint32_t> meshletTriangleIndices;
};

} // namespace Gleam
