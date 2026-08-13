#pragma once
#include "Renderer/MeshDescriptor.h"
#include "Math/Float4x4.h"

namespace GEditor {

struct RawMesh;

struct MeshLodData
{
	Gleam::TString name;
	Gleam::TArray<uint32_t> indices;
	Gleam::TArray<Gleam::Float3> positions;
	Gleam::TArray<Gleam::InterleavedMeshVertex> interleavedVertices;
	Gleam::TArray<Gleam::MeshletDescriptor> meshlets;
	Gleam::TArray<uint32_t> meshletVertices;
	Gleam::TArray<uint32_t> meshletTriangleIndices;
	Gleam::TArray<Gleam::SubmeshDescriptor> submeshes;
};

namespace MeshTools {

MeshLodData CombineMeshes(const Gleam::TArray<RawMesh>& meshes);
Gleam::TArray<Gleam::InterleavedMeshVertex> InterleaveMeshVertices(const RawMesh& mesh);
Gleam::BoundingBox CalculateBounds(const Gleam::TArray<Gleam::Float3>& positions);

void RemoveDegenerateFaces(RawMesh& mesh);
void ComputeSmoothNormals(RawMesh& mesh);
void ComputeTangents(RawMesh& mesh);
void ValidateTangents(RawMesh& mesh);
void ApplyTransform(RawMesh& mesh, const Gleam::Float4x4& transform);

} // namespace MeshTools

} // namespace GEditor
