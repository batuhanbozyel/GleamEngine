#include "Gleam.h"
#include "Model.h"

#define CGLTF_IMPLEMENTATION
#include <cgltf.h>

using namespace GEditor;

Model Model::Import(const Gleam::Filesystem::path& path)
{
	cgltf_options options = {};
	cgltf_data* data = nullptr;
	cgltf_result result = cgltf_parse_file(&options, path.string().c_str(), &data);
	if (result != cgltf_result_success)
	{
		return {};
	}

	Model model;
	for (uint32_t i = 0; i < data->meshes_count; ++i)
	{
		cgltf_mesh* mesh = &data->meshes[i];
		for (uint32_t j = 0; j < mesh->primitives_count; ++j)
		{
			const auto& primitive = mesh->primitives[j];
			uint32_t vertex_count = primitive.attributes_count ? primitive.attributes[0].data->count : 0;

			Gleam::MeshData meshData;

			// process indices
			if (primitive.indices)
			{
				meshData.indices.resize(primitive.indices->count);
				cgltf_accessor_unpack_indices(primitive.indices, meshData.indices.data(), meshData.indices.size());
			}
			else
			{
				meshData.indices.resize(vertex_count);
				for (uint32_t i = 0; i < vertex_count; ++i)
				{
					meshData.indices[i] = i;
				}
			}

			// process attributes
			for (uint32_t ai = 0; ai < primitive.attributes_count; ++ai)
			{
				const auto& attribute = primitive.attributes[ai];
				if (attribute.type == cgltf_attribute_type_position)
				{
					meshData.positions.resize(vertex_count);
					cgltf_accessor_unpack_floats(attribute.data, (cgltf_float*)meshData.positions.data(), meshData.positions.size());
				}
				else if (attribute.type == cgltf_attribute_type_normal)
				{
					meshData.normals.resize(vertex_count);
					cgltf_accessor_unpack_floats(attribute.data, (cgltf_float*)meshData.normals.data(), meshData.normals.size());
				}
				else if (attribute.type == cgltf_attribute_type_texcoord)
				{
					meshData.texCoords.resize(vertex_count);
					cgltf_accessor_unpack_floats(attribute.data, (cgltf_float*)meshData.texCoords.data(), meshData.texCoords.size());
				}
			}

			GLEAM_ASSERT(!meshData.positions.empty() && !meshData.normals.empty() && !meshData.texCoords.empty() && !meshData.indices.empty(), "glTF file does not meet the requirements!");
			model.mMeshes.push_back(meshData);
		}
	}
	cgltf_free(data);
	return model;
}

const Gleam::TArray<Gleam::MeshData>& Model::GetMeshes() const
{
    return mMeshes;
}
