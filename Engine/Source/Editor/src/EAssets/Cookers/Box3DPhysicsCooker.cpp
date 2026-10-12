#include "Box3DPhysicsCooker.h"

#include <box3d/box3d.h>
#include <cstring>

using namespace GEditor;

static_assert(sizeof(Gleam::Float3) == sizeof(b3Vec3), "Float3 must match b3Vec3 to alias vertices without a copy.");
static_assert(sizeof(b3MeshTriangle) == 3 * sizeof(uint32_t), "b3MeshTriangle must be three 32-bit indices.");

static const b3HullData* ToHullData(const Gleam::BinaryBuffer& buffer)
{
	const auto hull = static_cast<const b3HullData*>(buffer.data);
	if (buffer.size >= sizeof(b3HullData) and hull->version == B3_HULL_VERSION and static_cast<uint64_t>(hull->byteCount) == buffer.size)
	{
		return hull;
	}
	else
	{
		return nullptr;
	}
}

static const b3MeshData* ToMeshData(const Gleam::BinaryBuffer& buffer)
{
	const auto mesh = static_cast<const b3MeshData*>(buffer.data);
	if (buffer.size >= sizeof(b3MeshData) and mesh->version == B3_MESH_VERSION and static_cast<uint64_t>(mesh->byteCount) == buffer.size)
	{
		return mesh;
	}
	else
	{
		return nullptr;
	}
}

Gleam::AssetBackend Box3DPhysicsCooker::GetBackend() const
{
	return Gleam::AssetBackend::Box3D;
}

Gleam::BinaryBuffer Box3DPhysicsCooker::CookConvexHull(Gleam::TArrayView<const Gleam::Float3> points) const
{
	Gleam::BinaryBuffer buffer;
	b3HullData* hull = b3CreateHull(reinterpret_cast<const b3Vec3*>(points.data()), static_cast<int>(points.size()), B3_MAX_HULL_VERTICES);
	if (hull != nullptr)
	{
		buffer = Gleam::BinaryBuffer(hull, hull->byteCount);
		b3DestroyHull(hull);
	}
	return buffer;
}

Gleam::BinaryBuffer Box3DPhysicsCooker::CookTriangleMesh(const TriangleMeshData& mesh) const
{
	b3MeshDef def = {};
	def.vertices = const_cast<b3Vec3*>(reinterpret_cast<const b3Vec3*>(mesh.positions.data()));
	def.indices = const_cast<int32_t*>(reinterpret_cast<const int32_t*>(mesh.indices.data()));
	def.vertexCount = static_cast<int>(mesh.positions.size());
	def.triangleCount = static_cast<int>(mesh.indices.size() / 3);
	def.identifyEdges = true;

	Gleam::BinaryBuffer buffer;
	b3MeshData* meshData = b3CreateMesh(&def, nullptr, 0);
	if (meshData != nullptr)
	{
		buffer = Gleam::BinaryBuffer(meshData, meshData->byteCount);
		b3DestroyMesh(meshData);
	}
	return buffer;
}

TriangleMeshData Box3DPhysicsCooker::DecodeConvexHull(const Gleam::BinaryBuffer& hull) const
{
	TriangleMeshData result;
	if (const b3HullData* hullData = ToHullData(hull))
	{
		const b3Vec3* points = b3GetHullPoints(hullData);
		const b3HullHalfEdge* edges = b3GetHullEdges(hullData);
		const b3HullFace* faces = b3GetHullFaces(hullData);

		result.positions.resize(hullData->vertexCount);
		std::memcpy(result.positions.data(), points, hullData->vertexCount * sizeof(b3Vec3));

		for (int i = 0; i < hullData->faceCount; ++i)
		{
			const uint8_t first = faces[i].edge;
			uint8_t edge = edges[first].next;
			uint8_t next = edges[edge].next;
			while (next != first)
			{
				result.indices.push_back(edges[first].origin);
				result.indices.push_back(edges[edge].origin);
				result.indices.push_back(edges[next].origin);
				edge = next;
				next = edges[next].next;
			}
		}
	}
	return result;
}

TriangleMeshData Box3DPhysicsCooker::DecodeTriangleMesh(const Gleam::BinaryBuffer& mesh) const
{
	TriangleMeshData result;
	if (const b3MeshData* meshData = ToMeshData(mesh))
	{
		result.positions.resize(meshData->vertexCount);
		std::memcpy(result.positions.data(), b3GetMeshVertices(meshData), meshData->vertexCount * sizeof(b3Vec3));

		result.indices.resize(meshData->triangleCount * 3);
		std::memcpy(result.indices.data(), b3GetMeshTriangles(meshData), meshData->triangleCount * sizeof(b3MeshTriangle));
	}
	return result;
}
