#pragma once
#include "Renderer/MeshDescriptor.h"
#include "Math/Float4x4.h"

namespace GEditor {

struct RawMesh;

struct ConvexHullData
{
	Gleam::TArray<Gleam::Float3> positions;
	Gleam::TArray<uint32_t> indices;
};

struct MeshData
{
	Gleam::TString name;
	Gleam::BinaryBuffer buffer;
	Gleam::BufferRange indices;
	Gleam::BufferRange positions;
	Gleam::BufferRange interleavedVertices;
	Gleam::BufferRange meshlets;
	Gleam::BufferRange meshletVertices;
	Gleam::BufferRange meshletTriangleIndices;
	Gleam::TArray<Gleam::SubmeshDescriptor> submeshes;
};

struct ConvexDecompositionSettings
{
	uint32_t maxConvexHulls = 64;
	uint32_t maxVerticesPerHull = 64;
	uint32_t resolution = 400000;
	float minimumVolumePercentError = 1.0f;
	bool shrinkWrap = true;
};

namespace MeshTools {

MeshData CombineMeshes(const Gleam::TArray<RawMesh>& meshes);
Gleam::TArray<ConvexHullData> DecomposeConvex(const RawMesh& mesh, const ConvexDecompositionSettings& settings);
Gleam::TArray<Gleam::InterleavedMeshVertex> InterleaveMeshVertices(const RawMesh& mesh);
Gleam::BoundingBox CalculateBounds(const Gleam::TArray<Gleam::Float3>& positions);

void RemoveDegenerateFaces(RawMesh& mesh);
void ComputeSmoothNormals(RawMesh& mesh);
void ComputeTangents(RawMesh& mesh);
void ValidateTangents(RawMesh& mesh);
void ApplyTransform(RawMesh& mesh, const Gleam::Float4x4& transform);

} // namespace MeshTools

} // namespace GEditor
