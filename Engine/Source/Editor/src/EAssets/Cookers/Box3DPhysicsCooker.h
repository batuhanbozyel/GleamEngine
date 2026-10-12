#pragma once
#include "PhysicsCooker.h"

namespace GEditor {

class Box3DPhysicsCooker final : public PhysicsCooker
{
public:

	virtual Gleam::AssetBackend GetBackend() const override;

	virtual Gleam::BinaryBuffer CookConvexHull(Gleam::TArrayView<const Gleam::Float3> points) const override;

	virtual Gleam::BinaryBuffer CookTriangleMesh(const TriangleMeshData& mesh) const override;

	virtual TriangleMeshData DecodeConvexHull(const Gleam::BinaryBuffer& hull) const override;

	virtual TriangleMeshData DecodeTriangleMesh(const Gleam::BinaryBuffer& mesh) const override;

};

} // namespace GEditor
