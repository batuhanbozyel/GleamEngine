#include "MeshTools.h"
#include "EAssets/MeshSource.h"

#include <mikktspace.h>
#include <meshoptimizer.h>

using namespace GEditor;

namespace MikkT {

static int getNumFaces(const SMikkTSpaceContext* context)
{
	auto mesh = static_cast<RawMesh*>(context->m_pUserData);
	return static_cast<int>(mesh->indices.size() / 3);
}

static int getNumVerticesOfFace(const SMikkTSpaceContext* context, int faceIdx)
{
	return 3; // We're always using triangles
}

static void getPosition(const SMikkTSpaceContext* context, float outpos[], int faceIdx, int vertIdx)
{
	const auto mesh = static_cast<const RawMesh*>(context->m_pUserData);
	const auto& pos = mesh->positions[mesh->indices[faceIdx * 3 + vertIdx]];
	outpos[0] = pos.x;
	outpos[1] = pos.y;
	outpos[2] = pos.z;
}

static void getNormal(const SMikkTSpaceContext* context, float outnormal[], int faceIdx, int vertIdx)
{
	const auto mesh = static_cast<const RawMesh*>(context->m_pUserData);
	const auto& normal = mesh->normals[mesh->indices[faceIdx * 3 + vertIdx]];
	outnormal[0] = normal.x;
	outnormal[1] = normal.y;
	outnormal[2] = normal.z;
}

static void getTexCoord(const SMikkTSpaceContext* context, float outuv[], int faceIdx, int vertIdx)
{
	const auto mesh = static_cast<const RawMesh*>(context->m_pUserData);
	const auto& uv = mesh->texCoords[mesh->indices[faceIdx * 3 + vertIdx]];
	outuv[0] = uv.x;
	outuv[1] = uv.y;
}

static void setTSpaceBasic(const SMikkTSpaceContext* context, const float inTangent[], float sign, int faceIdx, int vertIdx)
{
	auto mesh = static_cast<RawMesh*>(context->m_pUserData);
	auto& tangent = mesh->tangents[mesh->indices[faceIdx * 3 + vertIdx]];
	tangent.x = inTangent[0];
	tangent.y = inTangent[1];
	tangent.z = inTangent[2];
	tangent.w = -1.0f * sign;
}

} // namespace MikkT

MeshLodData MeshTools::CombineMeshes(const Gleam::TArray<RawMesh>& meshes)
{
    MeshLodData combined;
    combined.submeshes.resize(meshes.size());

	uint64_t totalIndexCount = 0;
	uint64_t totalVertexCount = 0;
	for (const auto& mesh : meshes)
	{
		totalIndexCount += mesh.indices.size();
		totalVertexCount += mesh.positions.size();
	}

	combined.indices.resize(totalIndexCount);
	combined.positions.resize(totalVertexCount);
	combined.interleavedVertices.resize(totalVertexCount);

    Gleam::SubmeshDescriptor submesh;
    for (uint32_t i = 0; i < meshes.size(); ++i)
    {
		combined.name = meshes[i].name;

        const auto& mesh = meshes[i];
		submesh.materialIndex = mesh.material;
        submesh.bounds = CalculateBounds(mesh.positions);
        submesh.indexCount = static_cast<uint32_t>(mesh.indices.size());
		submesh.vertexCount = static_cast<uint32_t>(mesh.positions.size());
        combined.submeshes[i] = submesh;

        auto interleaved = InterleaveMeshVertices(mesh);
        memcpy(combined.indices.data() + submesh.firstIndex, mesh.indices.data(), mesh.indices.size() * sizeof(uint32_t));
        memcpy(combined.positions.data() + submesh.baseVertex, mesh.positions.data(), mesh.positions.size() * sizeof(Gleam::Float3));
        memcpy(combined.interleavedVertices.data() + submesh.baseVertex, interleaved.data(), interleaved.size() * sizeof(Gleam::InterleavedMeshVertex));

        submesh.baseVertex += static_cast<uint32_t>(mesh.positions.size());
        submesh.firstIndex += static_cast<uint32_t>(mesh.indices.size());
    }
	return combined;
}

Gleam::TArray<Gleam::InterleavedMeshVertex> MeshTools::InterleaveMeshVertices(const RawMesh& mesh)
{
	Gleam::TArray<Gleam::InterleavedMeshVertex> interleaved(mesh.normals.size());
	for (uint32_t i = 0; i < mesh.normals.size(); ++i)
	{
		interleaved[i].normal = mesh.normals[i];
		interleaved[i].tangent = mesh.tangents[i];
		interleaved[i].texCoord = mesh.texCoords[i];
		interleaved[i].color = mesh.colors[i];
	}
	return interleaved;
}

Gleam::BoundingBox MeshTools::CalculateBounds(const Gleam::TArray<Gleam::Float3>& positions)
{
    Gleam::BoundingBox bounds(Gleam::Math::Infinity, Gleam::Math::NegativeInfinity);
    for (const auto& position : positions)
    {
        bounds.min = Gleam::Math::Min(bounds.min, position);
        bounds.max = Gleam::Math::Max(bounds.max, position);
    }
    return bounds;
}

void MeshTools::RemoveDegenerateFaces(RawMesh& mesh)
{
	Gleam::TArray<uint32_t> newIndices;
	newIndices.reserve(mesh.indices.size());

	for (size_t i = 0; i < mesh.indices.size(); i += 3)
	{
		uint32_t i0 = mesh.indices[i];
		uint32_t i1 = mesh.indices[i + 1];
		uint32_t i2 = mesh.indices[i + 2];

		const Gleam::Float3& v0 = mesh.positions[i0];
		const Gleam::Float3& v1 = mesh.positions[i1];
		const Gleam::Float3& v2 = mesh.positions[i2];

		Gleam::Float3 edge1 = v1 - v0;
		Gleam::Float3 edge2 = v2 - v0;

		Gleam::Float3 cross = Gleam::Math::Cross(edge1, edge2);
		double area = Gleam::Math::Length(cross) * 0.5;

		if (area > Gleam::Math::SmallEpsilon)
		{
			newIndices.push_back(i0);
			newIndices.push_back(i1);
			newIndices.push_back(i2);
		}
	}

	if (newIndices.size() == mesh.indices.size())
	{
		return;
	}

	Gleam::TArray<bool> vertexUsed(mesh.positions.size(), false);
	for (uint32_t index : newIndices)
	{
		vertexUsed[index] = true;
	}

	Gleam::TArray<uint32_t> remapping(mesh.positions.size());
	uint32_t newVertexCount = 0;
	for (size_t i = 0; i < vertexUsed.size(); ++i)
	{
		if (vertexUsed[i])
		{
			remapping[i] = newVertexCount++;
		}
	}

	for (uint32_t& index : newIndices)
	{
		index = remapping[index];
	}

	Gleam::TArray<Gleam::Float3> newPositions;
	Gleam::TArray<Gleam::Float3> newNormals;
	Gleam::TArray<Gleam::Float4> newTangents;
	Gleam::TArray<Gleam::Float2> newTexCoords;
	Gleam::TArray<Gleam::Float4> newColors;

	newPositions.reserve(newVertexCount);

	if (not mesh.normals.empty())
	{
		newNormals.reserve(newVertexCount);
	}

	if (not mesh.tangents.empty())
	{
		newTangents.reserve(newVertexCount);
	}

	if (not mesh.texCoords.empty())
	{
		newTexCoords.reserve(newVertexCount);
	}

	if (not mesh.colors.empty())
	{
		newColors.reserve(newVertexCount);
	}

	for (size_t i = 0; i < vertexUsed.size(); ++i)
	{
		if (vertexUsed[i])
		{
			newPositions.push_back(mesh.positions[i]);

			if (not mesh.normals.empty())
			{
				newNormals.push_back(mesh.normals[i]);
			}

			if (not mesh.tangents.empty())
			{
				newTangents.push_back(mesh.tangents[i]);
			}

			if (not mesh.texCoords.empty())
			{
				newTexCoords.push_back(mesh.texCoords[i]);
			}

			if (not mesh.colors.empty())
			{
				newColors.push_back(mesh.colors[i]);
			}
		}
	}

	mesh.indices = std::move(newIndices);
	mesh.positions = std::move(newPositions);

	if (not mesh.normals.empty())
	{
		mesh.normals = std::move(newNormals);
	}

	if (not mesh.tangents.empty())
	{
		mesh.tangents = std::move(newTangents);
	}

	if (not mesh.texCoords.empty())
	{
		mesh.texCoords = std::move(newTexCoords);
	}

	if (not mesh.colors.empty())
	{
		mesh.colors = std::move(newColors);
	}
}

void MeshTools::ComputeSmoothNormals(RawMesh& mesh)
{
	mesh.normals.resize(mesh.positions.size());
	for (size_t i = 0; i < mesh.indices.size(); i += 3)
	{
		uint32_t i0 = mesh.indices[i];
		uint32_t i1 = mesh.indices[i + 1];
		uint32_t i2 = mesh.indices[i + 2];

		const Gleam::Float3& v0 = mesh.positions[i0];
		const Gleam::Float3& v1 = mesh.positions[i1];
		const Gleam::Float3& v2 = mesh.positions[i2];

		Gleam::Float3 edge1 = v1 - v0;
		Gleam::Float3 edge2 = v2 - v0;
		Gleam::Float3 faceNormal = Gleam::Math::Cross(edge1, edge2);

		mesh.normals[i0] += faceNormal;
		mesh.normals[i1] += faceNormal;
		mesh.normals[i2] += faceNormal;
	}

	for (auto& normal : mesh.normals)
	{
		if (Gleam::Math::LengthSquared(normal) > Gleam::Math::Epsilon)
		{
			normal = Gleam::Math::Normalize(normal);
		}
		else
		{
			normal = Gleam::Float3(0.0f, 1.0f, 0.0f);
		}
	}
}

void MeshTools::ValidateTangents(RawMesh& mesh)
{
	for (uint32_t i = 0; i < mesh.tangents.size(); ++i)
	{
		auto& tangent = mesh.tangents[i];
		const auto& normal = mesh.normals[i];

		Gleam::Float3 t(tangent.x, tangent.y, tangent.z);
		if (Gleam::Math::LengthSquared(Gleam::Math::Cross(normal, t)) < Gleam::Math::Epsilon)
		{
			Gleam::Float3 up = Gleam::Math::Abs(normal.y) < 0.999f ? Gleam::Float3(0.0f, 1.0f, 0.0f) : Gleam::Float3(1.0f, 0.0f, 0.0f);
			t = Gleam::Math::Normalize(Gleam::Math::Cross(up, normal));
			tangent = Gleam::Float4(t.x, t.y, t.z, tangent.w != 0.0f ? tangent.w : 1.0f);
		}
	}
}

void MeshTools::ComputeTangents(RawMesh& mesh)
{
	SMikkTSpaceInterface mikktInterface = {};
	mikktInterface.m_getNumFaces = &MikkT::getNumFaces;
	mikktInterface.m_getNumVerticesOfFace = &MikkT::getNumVerticesOfFace;
	mikktInterface.m_getPosition = &MikkT::getPosition;
	mikktInterface.m_getNormal = &MikkT::getNormal;
	mikktInterface.m_getTexCoord = &MikkT::getTexCoord;
	mikktInterface.m_setTSpaceBasic = &MikkT::setTSpaceBasic;

	SMikkTSpaceContext context = {};
	context.m_pInterface = &mikktInterface;
	context.m_pUserData = &mesh;
	mesh.tangents.resize(mesh.normals.size());
	if (genTangSpaceDefault(&context) == false)
	{
		mesh.tangents.clear();
		mesh.tangents.resize(mesh.normals.size(), Gleam::Float4(1.0f, 0.0f, 0.0f, 1.0f));
	}
}

void MeshTools::ApplyTransform(RawMesh& mesh, const Gleam::Float4x4& transform)
{
	const Gleam::Float3 xAxis(transform.m[0], transform.m[1], transform.m[2]);
	const Gleam::Float3 yAxis(transform.m[4], transform.m[5], transform.m[6]);
	const Gleam::Float3 zAxis(transform.m[8], transform.m[9], transform.m[10]);

	const bool mirrored = Gleam::Math::Dot(Gleam::Math::Cross(xAxis, yAxis), zAxis) < 0.0f;
	const float cofactorSign = mirrored ? -1.0f : 1.0f;
	const Gleam::Float3 normalX = Gleam::Math::Cross(yAxis, zAxis) * cofactorSign;
	const Gleam::Float3 normalY = Gleam::Math::Cross(zAxis, xAxis) * cofactorSign;
	const Gleam::Float3 normalZ = Gleam::Math::Cross(xAxis, yAxis) * cofactorSign;

	for (auto& position : mesh.positions)
	{
		position = transform * position;
	}

	for (auto& normal : mesh.normals)
	{
		normal = Gleam::Math::Normalize(normalX * normal.x + normalY * normal.y + normalZ * normal.z);
	}

	for (auto& tangent : mesh.tangents)
	{
		Gleam::Float3 t = Gleam::Math::Normalize(xAxis * tangent.x + yAxis * tangent.y + zAxis * tangent.z);
		tangent = Gleam::Float4(t.x, t.y, t.z, mirrored ? -tangent.w : tangent.w);
	}

	if (mirrored)
	{
		for (size_t i = 0; i + 2 < mesh.indices.size(); i += 3)
		{
			std::swap(mesh.indices[i + 1], mesh.indices[i + 2]);
		}
	}
}
