#pragma once
#include "Renderer/MeshDescriptor.h"

namespace GEditor {

struct RawMesh;

namespace MeshTools {

Gleam::MeshDescriptor CombineMeshes(const Gleam::TArray<RawMesh>& meshes);
Gleam::TArray<Gleam::InterleavedMeshVertex> InterleaveMeshVertices(const RawMesh& mesh);
Gleam::BoundingBox CalculateBounds(const Gleam::TArray<Gleam::Float3>& positions);

void RemoveDegenerateFaces(RawMesh& mesh);
void ComputeSmoothNormals(RawMesh& mesh);
void ComputeTangents(RawMesh& mesh);
void ValidateTangents(RawMesh& mesh);

} // namespace MeshTools

} // namespace GEditor
