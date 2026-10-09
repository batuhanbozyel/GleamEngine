#pragma once
#include "Renderer/MeshDescriptor.h"
#include "Math/Float4x4.h"
#include "Core/Attributes.h"

namespace GEditor {

struct RawMesh;

struct ConvexHullData
{
	Gleam::TArray<Gleam::Float3> positions;
};

struct TriangleMeshData
{
	Gleam::TArray<Gleam::Float3> positions;
	Gleam::TArray<uint32_t> indices;
};

struct MeshLodData
{
	Gleam::BinaryBuffer buffer;
	Gleam::BufferRange indices;
	Gleam::BufferRange positions;
	Gleam::BufferRange interleavedVertices;
	Gleam::BufferRange meshlets;
	Gleam::BufferRange meshletVertices;
	Gleam::BufferRange meshletTriangleIndices;
	Gleam::TArray<Gleam::SubmeshDescriptor> submeshes;
};

struct MeshData
{
	Gleam::TString name;
	Gleam::BoundingBox aabb;
	Gleam::TArray<MeshLodData> lods;
	Gleam::TArray<ConvexHullData> convexHulls;
	Gleam::TArray<TriangleMeshData> triangleMeshes;
};

GSTRUCT(ConvexDecompositionSettings, "6DFB87AB-8AA9-4206-AE6A-6ACDBC47589A", Serializable, PrettyName("Convex Decomposition"))
{
	GFIELD("159D6C0D-FF03-4B7D-978B-DEAFB82EEEBA", Serializable, PrettyName("Max Convex Hulls"))
	uint32_t maxConvexHulls = 64;

	GFIELD("FD45F4F7-77A9-4D88-ACCD-D62618C6904A", Serializable, PrettyName("Max Vertices Per Hull"))
	uint32_t maxVerticesPerHull = 64;

	GFIELD("C3F7F1D6-AFDA-41FF-93F9-0C855F7663FD", Serializable, PrettyName("Resolution"))
	uint32_t resolution = 400000;

	GFIELD("50FBFE39-00F4-4835-9A42-C9CF9D2DBA38", Serializable, PrettyName("Min Volume Error %"))
	float minimumVolumePercentError = 1.0f;

	GFIELD("158AE83A-A8A2-4FF1-96CA-DC89BD446E51", Serializable, PrettyName("Shrink Wrap"))
	bool shrinkWrap = true;
};

GSTRUCT(TriangleMeshSimplificationSettings, "E2F3DCF2-76C7-4C7E-9D1F-14D5301976A4", Serializable, PrettyName("Triangle Mesh Simplification"))
{
	GFIELD("4BBFC7E2-A697-49B2-AA56-00E332EF38FB", Serializable, PrettyName("Target Ratio"))
	float targetRatio = 0.25f;
};

namespace MeshTools {

MeshLodData CombineMeshes(const Gleam::TArray<RawMesh>& meshes);
MeshLodData SimplifyMesh(const MeshLodData& lod, float ratio);
TriangleMeshData SimplifyMeshSloppy(const MeshLodData& lod, const TriangleMeshSimplificationSettings& settings);
void BuildMeshlets(MeshLodData& lodData);
Gleam::TArray<ConvexHullData> DecomposeConvex(const MeshLodData& lod, const ConvexDecompositionSettings& settings);
Gleam::BoundingBox CalculateBounds(Gleam::TArrayView<const Gleam::Float3> positions);

void RemoveDegenerateFaces(RawMesh& mesh);
void ComputeSmoothNormals(RawMesh& mesh);
void ComputeTangents(RawMesh& mesh);
void ValidateTangents(RawMesh& mesh);
void ApplyTransform(RawMesh& mesh, const Gleam::Float4x4& transform);

} // namespace MeshTools

} // namespace GEditor
