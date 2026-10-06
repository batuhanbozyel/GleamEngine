#include "MeshTools.h"
#include "EAssets/MeshSource.h"
#include "Renderer/Shaders/ShaderInterop.h"

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
        meshopt_optimizeVertexCache(indices + submesh.firstIndex, indices + submesh.firstIndex, mesh.indices.size(), mesh.positions.size());

        submesh.baseVertex += static_cast<uint32_t>(mesh.positions.size());
        submesh.firstIndex += static_cast<uint32_t>(mesh.indices.size());
    }
	return combined;
}

MeshLodData MeshTools::SimplifyMesh(const MeshLodData& lod, float ratio)
{
	static constexpr float targetError = 1.0f;
	static constexpr float kNormalWeight = 0.5f;
	static constexpr float kTangentWeight = 0.0f;
	static constexpr float kTexCoordWeight = 0.5f;
	static constexpr float kColorWeight = 0.5f;
	static constexpr float kAttrWeights[] = { kNormalWeight, kNormalWeight, kNormalWeight,
											  kTangentWeight, kTangentWeight, kTangentWeight, kTangentWeight,
											  kTexCoordWeight, kTexCoordWeight,
											  kColorWeight, kColorWeight, kColorWeight, kColorWeight };
	static constexpr size_t kAttributeCount = eastl::size(kAttrWeights);
	static_assert(sizeof(Gleam::InterleavedMeshVertex) == kAttributeCount * sizeof(float), "Attribute weights must match InterleavedMeshVertex layout");

	const auto sourceIndices = static_cast<const uint32_t*>(Gleam::OffsetPointer(lod.buffer.data, lod.indices.offset));
	const auto sourcePositions = static_cast<const Gleam::Float3*>(Gleam::OffsetPointer(lod.buffer.data, lod.positions.offset));
	const auto sourceVertices = static_cast<const Gleam::InterleavedMeshVertex*>(Gleam::OffsetPointer(lod.buffer.data, lod.interleavedVertices.offset));

	MeshLodData simplified;
	simplified.submeshes.resize(lod.submeshes.size());
	simplified.buffer = Gleam::BinaryBuffer(lod.indices.size + lod.positions.size + lod.interleavedVertices.size);
	auto indices = static_cast<uint32_t*>(simplified.buffer.data);
	auto positions = static_cast<Gleam::Float3*>(Gleam::OffsetPointer(simplified.buffer.data, lod.indices.size));
	auto vertices = static_cast<Gleam::InterleavedMeshVertex*>(Gleam::OffsetPointer(simplified.buffer.data, lod.indices.size + lod.positions.size));

	uint32_t firstIndex = 0;
	uint32_t baseVertex = 0;
	Gleam::TArray<uint32_t> remap;
	for (uint32_t i = 0; i < lod.submeshes.size(); ++i)
	{
		const auto& source = lod.submeshes[i];
		const size_t targetIndexCount = static_cast<size_t>(source.indexCount * ratio) / 3 * 3;
		if (targetIndexCount > 196) // 64 triangles minimum
		{
			const auto inIndices = sourceIndices + source.firstIndex;
			const auto inPositions = sourcePositions + source.baseVertex;
			const auto inVertices = sourceVertices + source.baseVertex;
			auto outIndices = indices + firstIndex;
			auto outPositions = positions + baseVertex;
			auto outVertices = vertices + baseVertex;

			const meshopt_Stream vertexStreams[] = {
					meshopt_Stream{.data = inPositions, .size = sizeof(Gleam::Float3), .stride = sizeof(Gleam::Float3)},
					meshopt_Stream{.data = inVertices, .size = sizeof(Gleam::InterleavedMeshVertex), .stride = sizeof(Gleam::InterleavedMeshVertex)},
			};

			remap.resize(source.vertexCount);
			const size_t weldedVertexCount = meshopt_generateVertexRemapMulti(remap.data(), inIndices, source.indexCount, source.vertexCount, vertexStreams, eastl::size(vertexStreams));
			meshopt_remapIndexBuffer(outIndices, inIndices, source.indexCount, remap.data());
			meshopt_remapVertexBuffer(outPositions, inPositions, source.vertexCount, sizeof(Gleam::Float3), remap.data());
			meshopt_remapVertexBuffer(outVertices, inVertices, source.vertexCount, sizeof(Gleam::InterleavedMeshVertex), remap.data());

			const size_t indexCount = meshopt_simplifyWithAttributes(outIndices,
																	 outIndices,
																	 source.indexCount,
																	 (const float*)outPositions,
																	 weldedVertexCount,
																	 sizeof(Gleam::Float3),
																	 (const float*)outVertices,
																	 sizeof(Gleam::InterleavedMeshVertex),
																	 kAttrWeights,
																	 kAttributeCount,
																	 nullptr,
																	 targetIndexCount,
																	 targetError,
																	 0,
																	 nullptr);

			if (indexCount > 0)
			{
				const size_t vertexCount = meshopt_optimizeVertexFetchRemap(remap.data(), outIndices, indexCount, weldedVertexCount);
				meshopt_remapIndexBuffer(outIndices, outIndices, indexCount, remap.data());
				meshopt_remapVertexBuffer(outPositions, outPositions, weldedVertexCount, sizeof(Gleam::Float3), remap.data());
				meshopt_remapVertexBuffer(outVertices, outVertices, weldedVertexCount, sizeof(Gleam::InterleavedMeshVertex), remap.data());
				meshopt_optimizeVertexCache(outIndices, outIndices, indexCount, vertexCount);

				auto& submesh = simplified.submeshes[i];
				submesh.materialIndex = source.materialIndex;
				submesh.firstIndex = firstIndex;
				submesh.baseVertex = baseVertex;
				submesh.indexCount = static_cast<uint32_t>(indexCount);
				submesh.vertexCount = static_cast<uint32_t>(vertexCount);
				submesh.bounds = CalculateBounds(Gleam::TArrayView<const Gleam::Float3>(outPositions, vertexCount));

				firstIndex += submesh.indexCount;
				baseVertex += submesh.vertexCount;
			}
		}
	}

	const uint64_t indexBufferSize = firstIndex * sizeof(uint32_t);
	const uint64_t positionBufferSize = baseVertex * sizeof(Gleam::Float3);
	const uint64_t interleavedBufferSize = baseVertex * sizeof(Gleam::InterleavedMeshVertex);
	memmove(Gleam::OffsetPointer(simplified.buffer.data, indexBufferSize), positions, positionBufferSize);
	memmove(Gleam::OffsetPointer(simplified.buffer.data, indexBufferSize + positionBufferSize), vertices, interleavedBufferSize);

	simplified.indices = { 0, indexBufferSize };
	simplified.positions = { indexBufferSize, positionBufferSize };
	simplified.interleavedVertices = { indexBufferSize + positionBufferSize, interleavedBufferSize };
	simplified.buffer.Resize(indexBufferSize + positionBufferSize + interleavedBufferSize);
	return simplified;
}

void MeshTools::BuildMeshlets(MeshLodData& lodData)
{
	static constexpr uint32_t kMaxVerticesPerMeshlet = MAX_MESHLET_VERTICES;
	static constexpr uint32_t kMaxTrianglesPerMeshlet = MAX_MESHLET_TRIANGLES;
	static constexpr float kConeWeight = 0.25f;

	Gleam::TArray<Gleam::MeshletDescriptor> combinedMeshlets;
	Gleam::TArray<uint32_t> combinedMeshletVertices;
	Gleam::TArray<uint32_t> combinedMeshletTriangles;

	size_t totalIndexCount = 0;
	for (const auto& submesh : lodData.submeshes)
	{
		totalIndexCount += submesh.indexCount;
	}
	combinedMeshletTriangles.reserve(totalIndexCount / 3);
	combinedMeshletVertices.reserve(totalIndexCount);

	auto combinedIndices = static_cast<uint32_t*>(Gleam::OffsetPointer(lodData.buffer.data, lodData.indices.offset));
	auto combinedPositions = static_cast<Gleam::Float3*>(Gleam::OffsetPointer(lodData.buffer.data, lodData.positions.offset));
	for (auto& submesh : lodData.submeshes)
	{
		Gleam::TArrayView<uint32_t> indices(combinedIndices + submesh.firstIndex, submesh.indexCount);
		Gleam::TArrayView<Gleam::Float3> positions(combinedPositions + submesh.baseVertex, submesh.vertexCount);

		size_t maxMeshlets = meshopt_buildMeshletsBound(indices.size(), kMaxVerticesPerMeshlet, kMaxTrianglesPerMeshlet);
		Gleam::TArray<meshopt_Meshlet> meshlets(maxMeshlets);
		Gleam::TArray<uint32_t> meshletVertices(indices.size());
		Gleam::TArray<uint8_t> meshletTriangleIndices(indices.size());
		size_t meshletCount = meshopt_buildMeshlets(meshlets.data(),
													meshletVertices.data(),
													meshletTriangleIndices.data(),
													indices.data(),
													indices.size(),
													(float*)positions.data(),
													positions.size(),
													sizeof(Gleam::Float3),
													kMaxVerticesPerMeshlet,
													kMaxTrianglesPerMeshlet,
													kConeWeight);

		const auto& last = meshlets[meshletCount - 1];
		meshletVertices.resize(last.vertex_offset + last.vertex_count);
		meshletTriangleIndices.resize(last.triangle_offset + last.triangle_count * 3);

		submesh.baseMeshlet = static_cast<uint32_t>(combinedMeshlets.size());
		submesh.meshletCount = static_cast<uint32_t>(meshletCount);
		combinedMeshlets.resize(combinedMeshlets.size() + meshletCount);
		for (uint32_t i = 0; i < meshletCount; ++i)
		{
			const auto& meshlet = meshlets[i];
			auto& meshletDesc = combinedMeshlets[submesh.baseMeshlet + i];

			auto meshletVerticesData = meshletVertices.data() + meshlet.vertex_offset;
			auto meshletTriangleData = meshletTriangleIndices.data() + meshlet.triangle_offset;
			
			meshopt_optimizeMeshlet(meshletVerticesData,
									meshletTriangleData,
									meshlet.triangle_count,
									meshlet.vertex_count);

			meshletDesc.vertexOffset = static_cast<uint32_t>(combinedMeshletVertices.size() + meshlet.vertex_offset);
			meshletDesc.triangleOffset = static_cast<uint32_t>(combinedMeshletTriangles.size());
			meshletDesc.vertexCount = static_cast<uint32_t>(meshlet.vertex_count);
			meshletDesc.triangleCount = static_cast<uint32_t>(meshlet.triangle_count);

			for (uint32_t t = 0; t < meshlet.triangle_count; ++t)
			{
				uint32_t packedTriangle = static_cast<uint32_t>(meshletTriangleData[t * 3 + 0])
										| (static_cast<uint32_t>(meshletTriangleData[t * 3 + 1]) << 8)
										| (static_cast<uint32_t>(meshletTriangleData[t * 3 + 2]) << 16);
				combinedMeshletTriangles.push_back(packedTriangle);
			}

			meshopt_Bounds bounds = meshopt_computeMeshletBounds(meshletVerticesData,
																 meshletTriangleData,
																 meshlet.triangle_count,
																 (float*)positions.data(),
																 positions.size(),
																 sizeof(Gleam::Float3));
			meshletDesc.coneApex = Gleam::Float3(bounds.cone_apex[0], bounds.cone_apex[1], bounds.cone_apex[2]);
			meshletDesc.coneAxis = Gleam::Float3(bounds.cone_axis[0], bounds.cone_axis[1], bounds.cone_axis[2]);
			meshletDesc.coneCutoff = bounds.cone_cutoff;
			meshletDesc.center = Gleam::Float3(bounds.center[0], bounds.center[1], bounds.center[2]);
			meshletDesc.radius = bounds.radius;
		}

		combinedMeshletVertices.insert(combinedMeshletVertices.end(), meshletVertices.begin(), meshletVertices.end());
	}

	const uint64_t meshletBufferSize = combinedMeshlets.size() * sizeof(Gleam::MeshletDescriptor);
	const uint64_t meshletVertexBufferSize = combinedMeshletVertices.size() * sizeof(uint32_t);
	const uint64_t meshletTriangleBufferSize = combinedMeshletTriangles.size() * sizeof(uint32_t);

	const uint64_t meshletBufferOffset = lodData.buffer.size;
	lodData.buffer.Resize(meshletBufferOffset + meshletBufferSize + meshletVertexBufferSize + meshletTriangleBufferSize);

	lodData.meshlets = { meshletBufferOffset, meshletBufferSize };
	lodData.meshletVertices = { lodData.meshlets.offset + lodData.meshlets.size, meshletVertexBufferSize };
	lodData.meshletTriangleIndices = { lodData.meshletVertices.offset + lodData.meshletVertices.size, meshletTriangleBufferSize };

	memcpy(Gleam::OffsetPointer(lodData.buffer.data, lodData.meshlets.offset), combinedMeshlets.data(), meshletBufferSize);
	memcpy(Gleam::OffsetPointer(lodData.buffer.data, lodData.meshletVertices.offset), combinedMeshletVertices.data(), meshletVertexBufferSize);
	memcpy(Gleam::OffsetPointer(lodData.buffer.data, lodData.meshletTriangleIndices.offset), combinedMeshletTriangles.data(), meshletTriangleBufferSize);
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

Gleam::BoundingBox MeshTools::CalculateBounds(Gleam::TArrayView<const Gleam::Float3> positions)
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
