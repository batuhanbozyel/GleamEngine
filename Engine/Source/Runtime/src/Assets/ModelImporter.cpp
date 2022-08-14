#include "gpch.h"
#include "ModelImporter.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

namespace tinyobj {

bool operator==(const index_t& lhs, const index_t& rhs)
{
	return	lhs.vertex_index == rhs.vertex_index &&
			lhs.normal_index == rhs.normal_index &&
			lhs.texcoord_index == rhs.texcoord_index;
}

} // namespace tinyobj


using namespace Gleam;

Model ModelImporter::Import(const std::filesystem::path& path)
{
	const auto& ext = path.extension();
	if (ext == ".obj")
	{
		return ImportObj(path);
	}
	else
	{
		GLEAM_ASSERT(false, "Model file type is not supported!");
		return Model();
	}
}

Model ModelImporter::ImportObj(const std::filesystem::path& path)
{
	TString objFile = IOUtils::ReadFile(path.c_str());

	TString mtlPath;
	TString mtlFile;
	auto mtllibIdx = objFile.find("mtllib");
	if (mtllibIdx != std::string::npos)
	{
		auto mtlfileBeginIdx = mtllibIdx + 7;
		auto mtlfileEndIdx = objFile.find(".mtl", mtlfileBeginIdx);
		if (mtlfileEndIdx != std::string::npos)
		{
			auto mtlfileNameLength = mtlfileEndIdx - mtlfileBeginIdx + 4;
			auto pathStr = path.string();
			mtlPath = pathStr.substr(0, pathStr.find_last_of("/\\") + 1) + objFile.substr(mtlfileBeginIdx, mtlfileNameLength);
			mtlFile = IOUtils::ReadFile(mtlPath.c_str());
		}
	}

	tinyobj::ObjReader reader;
	if (!reader.ParseFromString(objFile, mtlFile))
	{
		if (!reader.Error().empty())
		{
			GLEAM_CORE_ERROR(("TinyObjReader: " + reader.Error()).c_str());
		}
		return Model();
	}

	if (!reader.Warning().empty())
	{
		GLEAM_CORE_WARN(("TinyObjReader: " + reader.Warning()).c_str());
	}

	const auto& attrib = reader.GetAttrib();
	const auto& shapes = reader.GetShapes();
	const auto& materials = reader.GetMaterials();

	auto HashIndices = [](const tinyobj::index_t& val)
	{
		int seed = 0xa5a8ae1e;
		const void* key = &val;
		int len = sizeof(val);

		const uint32_t m = 0x5bd1e995;
		const int32_t r = 24;
		uint32_t h = seed ^ len;
		const uint8_t* data = (const uint8_t*)key;

		while (len >= 4)
		{
			uint32_t k = *(uint32_t*)data;
			k *= m;
			k ^= k >> r;
			k *= m;
			h *= m;
			h ^= k;
			data += 4;
			len -= 4;
		}

		switch (len)
		{
			case 3: h ^= data[2] << 16;
			case 2: h ^= data[1] << 8;
			case 1: h ^= data[0];
				h *= m;
		};

		h ^= h >> 13;
		h *= m;
		h ^= h >> 15;

		return h;
	};

	TArray<MeshData> meshes(shapes.size());
	HashMap<tinyobj::index_t, uint32_t, decltype(HashIndices)> uniqueVertices(10, HashIndices);
	for (const auto& shape : shapes)
	{
		MeshData mesh;
		auto materialId = shape.mesh.material_ids[0]; // per face materials are not supported
		if (materials.size() > 0 && materialId >= 0)
		{
			auto directory = mtlPath.substr(0, mtlPath.find_last_of("/\\") + 1);
			mesh.material.baseTexture = directory + materials[materialId].diffuse_texname;
		}

		for (uint32_t face = 0; face < shape.mesh.num_face_vertices.size(); face++)
		{
			for (uint32_t v = 0; v < 3; v++)
			{
				const auto& index = shape.mesh.indices[face * 3 + v];
				auto it = uniqueVertices.find(index);
				if (it == uniqueVertices.end())
				{
					it = uniqueVertices.insert(uniqueVertices.end(), { index, static_cast<uint32_t>(mesh.positions.size()) });

					Vector3 position = {
						attrib.vertices[3 * index.vertex_index + 0],
						attrib.vertices[3 * index.vertex_index + 1],
						attrib.vertices[3 * index.vertex_index + 2]
					};
					mesh.positions.emplace_back(position);

					if (index.normal_index >= 0)
					{
						Vector3 normal = {
							attrib.normals[3 * index.normal_index + 0],
							attrib.normals[3 * index.normal_index + 1],
							attrib.normals[3 * index.normal_index + 2]
						};
						mesh.normals.emplace_back(normal);
					}

					Vector2 texCoord = { 0.0f, 0.0f };
					if (index.texcoord_index >= 0)
					{
						texCoord = {
							attrib.texcoords[2 * index.texcoord_index + 0],
							attrib.texcoords[2 * index.texcoord_index + 1]
						};
					}
					mesh.texCoords.emplace_back(texCoord);
				}
				mesh.indices.push_back(it->second);
			}
		}
 		meshes.emplace_back(mesh);
	}

	return Model(meshes);
}