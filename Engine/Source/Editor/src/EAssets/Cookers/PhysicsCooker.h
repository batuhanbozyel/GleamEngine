#pragma once
#include "EAssets/Tools/MeshTools.h"
#include "Assets/AssetHeader.h"

namespace GEditor {

class PhysicsCooker
{
public:

	virtual ~PhysicsCooker() = default;

	virtual Gleam::AssetBackend GetBackend() const = 0;

	virtual Gleam::BinaryBuffer CookConvexHull(Gleam::TArrayView<const Gleam::Float3> points) const = 0;

	virtual Gleam::BinaryBuffer CookTriangleMesh(const TriangleMeshData& mesh) const = 0;

	virtual TriangleMeshData DecodeConvexHull(const Gleam::BinaryBuffer& hull) const = 0;

	virtual TriangleMeshData DecodeTriangleMesh(const Gleam::BinaryBuffer& mesh) const = 0;

	static Gleam::Scope<PhysicsCooker> Create();

};

} // namespace GEditor
