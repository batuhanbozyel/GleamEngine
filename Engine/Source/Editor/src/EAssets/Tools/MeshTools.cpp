#include "MeshTools.h"
#include "EAssets/MeshSource.h"

#define ENABLE_VHACD_IMPLEMENTATION 1
#include <VHACD.h>
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

	const uint64_t indexBufferSize = totalIndexCount * sizeof(uint32_t);
	const uint64_t positionBufferSize = totalVertexCount * sizeof(Gleam::Float3);
	const uint64_t interleavedBufferSize = totalVertexCount * sizeof(Gleam::InterleavedMeshVertex);

	combined.buffer = Gleam::BinaryBuffer(indexBufferSize + positionBufferSize + interleavedBufferSize);
	combined.indices = { 0, indexBufferSize };
	combined.positions = { indexBufferSize, positionBufferSize };
	combined.interleavedVertices = { indexBufferSize + positionBufferSize, interleavedBufferSize };

	auto indices = static_cast<uint32_t*>(Gleam::OffsetPointer(combined.buffer.data, combined.indices.offset));
	auto positions = static_cast<Gleam::Float3*>(Gleam::OffsetPointer(combined.buffer.data, combined.positions.offset));
	auto interleavedVertices = static_cast<Gleam::InterleavedMeshVertex*>(Gleam::OffsetPointer(combined.buffer.data, combined.interleavedVertices.offset));

    Gleam::SubmeshDescriptor submesh;
    for (uint32_t i = 0; i < meshes.size(); ++i)
    {
        const auto& mesh = meshes[i];
		submesh.materialIndex = mesh.material;
        submesh.bounds = CalculateBounds(mesh.positions);
        submesh.indexCount = static_cast<uint32_t>(mesh.indices.size());
		submesh.vertexCount = static_cast<uint32_t>(mesh.positions.size());
        combined.submeshes[i] = submesh;

        auto interleaved = InterleaveMeshVertices(mesh);
        memcpy(indices + submesh.firstIndex, mesh.indices.data(), mesh.indices.size() * sizeof(uint32_t));
        memcpy(positions + submesh.baseVertex, mesh.positions.data(), mesh.positions.size() * sizeof(Gleam::Float3));
        memcpy(interleavedVertices + submesh.baseVertex, interleaved.data(), interleaved.size() * sizeof(Gleam::InterleavedMeshVertex));

        submesh.baseVertex += static_cast<uint32_t>(mesh.positions.size());
        submesh.firstIndex += static_cast<uint32_t>(mesh.indices.size());
    }
	return combined;
}

static RawMesh RemapMesh(const RawMesh& source, const Gleam::TArray<uint32_t>& indices, const Gleam::TArray<uint32_t>& remap, size_t vertexCount)
{
	const size_t sourceVertexCount = source.positions.size();

	RawMesh destination;
	destination.name = source.name;
	destination.aabb = source.aabb;
	destination.material = source.material;
	destination.indices.resize(indices.size());
	destination.positions.resize(vertexCount);
	destination.normals.resize(vertexCount);
	destination.tangents.resize(vertexCount);
	destination.texCoords.resize(vertexCount);
	destination.colors.resize(vertexCount);

	meshopt_remapIndexBuffer(destination.indices.data(), indices.data(), indices.size(), remap.data());
	meshopt_remapVertexBuffer(destination.positions.data(), source.positions.data(), sourceVertexCount, sizeof(Gleam::Float3), remap.data());
	meshopt_remapVertexBuffer(destination.normals.data(), source.normals.data(), sourceVertexCount, sizeof(Gleam::Float3), remap.data());
	meshopt_remapVertexBuffer(destination.tangents.data(), source.tangents.data(), sourceVertexCount, sizeof(Gleam::Float4), remap.data());
	meshopt_remapVertexBuffer(destination.texCoords.data(), source.texCoords.data(), sourceVertexCount, sizeof(Gleam::Float2), remap.data());
	meshopt_remapVertexBuffer(destination.colors.data(), source.colors.data(), sourceVertexCount, sizeof(Gleam::Float4), remap.data());
	return destination;
}

RawMesh MeshTools::SimplifyMesh(const RawMesh& mesh, float ratio, bool lockBorder)
{
	static constexpr float kTexCoordWeight = 1.0f;
	static constexpr float kColorWeigth = 1.0f;
	static constexpr float kNormalWeight = 0.5f;
	static constexpr float kTangentWeight = 0.5f;
	static constexpr float kAttrWeights[] = { kColorWeigth,	kColorWeigth, kColorWeigth, kColorWeigth,
											  kTangentWeight, kTangentWeight, kTangentWeight,  kTangentWeight,
											  kNormalWeight, kNormalWeight, kNormalWeight,
											  kTexCoordWeight, kTexCoordWeight };
	static constexpr size_t kAttributeCount = eastl::size(kAttrWeights);

	float targetError = 1.0f;
	size_t targetIndexCount = static_cast<size_t>(mesh.indices.size() * ratio) / 3 * 3;

	RawMesh simplified = {};
	if (targetIndexCount > 128)
	{
		int simplifyOptions = lockBorder ? meshopt_SimplifyLockBorder : 0;

		meshopt_Stream vertexStreams[] = {
				meshopt_Stream{.data = mesh.positions.data(), .size = sizeof(Gleam::Float3), .stride = sizeof(Gleam::Float3)},
				meshopt_Stream{.data = mesh.normals.data(), .size = sizeof(Gleam::Float3), .stride = sizeof(Gleam::Float3)},
				meshopt_Stream{.data = mesh.tangents.data(),.size = sizeof(Gleam::Float4), .stride = sizeof(Gleam::Float4)},
				meshopt_Stream{.data = mesh.colors.data(), .size = sizeof(Gleam::Float4), .stride = sizeof(Gleam::Float4)},
				meshopt_Stream{.data = mesh.texCoords.data(), .size = sizeof(Gleam::Float2), .stride = sizeof(Gleam::Float2)},
		};

		Gleam::TArray<uint32_t> remap(mesh.positions.size());
		const size_t weldedVertexCount = meshopt_generateVertexRemapMulti(remap.data(),
			mesh.indices.data(),
			mesh.indices.size(),
			mesh.positions.size(),
			vertexStreams,
			eastl::size(vertexStreams));
		const RawMesh welded = RemapMesh(mesh, mesh.indices, remap, weldedVertexCount);

		Gleam::TArray<float> attributes(weldedVertexCount * kAttributeCount);
		for (size_t i = 0; i < weldedVertexCount; ++i)
		{
			float* attribute = attributes.data() + i * kAttributeCount;
			memcpy(attribute, &welded.colors[i], sizeof(Gleam::Float4));
			memcpy(attribute + 4, &welded.tangents[i], sizeof(Gleam::Float4));
			memcpy(attribute + 8, &welded.normals[i], sizeof(Gleam::Float3));
			memcpy(attribute + 11, &welded.texCoords[i], sizeof(Gleam::Float2));
		}

		Gleam::TArray<uint32_t> indices(welded.indices.size());
		indices.resize(meshopt_simplifyWithAttributes(indices.data(),
													  welded.indices.data(),
													  welded.indices.size(),
													  (const float*)welded.positions.data(),
													  weldedVertexCount,
													  sizeof(Gleam::Float3),
													  attributes.data(),
													  kAttributeCount * sizeof(float),
													  kAttrWeights,
													  kAttributeCount,
													  nullptr,
													  targetIndexCount,
													  targetError,
													  simplifyOptions,
													  nullptr));

		if (not indices.empty())
		{
			remap.resize(weldedVertexCount);
			const size_t simplifiedVertexCount = meshopt_optimizeVertexFetchRemap(remap.data(), indices.data(), indices.size(), weldedVertexCount);
			simplified = RemapMesh(welded, indices, remap, simplifiedVertexCount);
		}
	}
	return simplified;
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
    Gleam::BoundingBox bounds;
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

Gleam::TArray<ConvexHullData> MeshTools::DecomposeConvex(const RawMesh& mesh, const ConvexDecompositionSettings& settings)
{
	// Box3D rejects hulls above B3_MAX_HULL_VERTICES, so never emit more than it can consume.
	static constexpr uint32_t kMaxHullVertices = 128;

	Gleam::TArray<ConvexHullData> hulls;
	VHACD::IVHACD::Parameters parameters;
	parameters.m_maxConvexHulls = settings.maxConvexHulls;
	parameters.m_maxNumVerticesPerCH = Gleam::Math::Min(settings.maxVerticesPerHull, kMaxHullVertices);
	parameters.m_resolution = settings.resolution;
	parameters.m_minimumVolumePercentErrorAllowed = settings.minimumVolumePercentError;
	parameters.m_shrinkWrap = settings.shrinkWrap;
	parameters.m_asyncACD = false;

	auto decomposer = VHACD::CreateVHACD();
	if (decomposer->Compute(reinterpret_cast<const float*>(mesh.positions.data()),
							static_cast<uint32_t>(mesh.positions.size()),
							mesh.indices.data(),
							static_cast<uint32_t>(mesh.indices.size() / 3),
							parameters))
	{
		const uint32_t hullCount = decomposer->GetNConvexHulls();
		hulls.reserve(hullCount);
		for (uint32_t i = 0; i < hullCount; ++i)
		{
			VHACD::IVHACD::ConvexHull hull;
			if (decomposer->GetConvexHull(i, hull) == false)
			{
				continue;
			}

			ConvexHullData& result = hulls.emplace_back();
			result.positions.reserve(hull.m_points.size());
			for (const auto& point : hull.m_points)
			{
				result.positions.emplace_back(static_cast<float>(point.mX),
											  static_cast<float>(point.mY),
											  static_cast<float>(point.mZ));
			}

			result.indices.reserve(hull.m_triangles.size() * 3);
			for (const auto& triangle : hull.m_triangles)
			{
				result.indices.push_back(triangle.mI0);
				result.indices.push_back(triangle.mI1);
				result.indices.push_back(triangle.mI2);
			}
		}
	}

	decomposer->Clean();
	decomposer->Release();
	return hulls;
}
