#include "Gleam.h"
#include "Model.h"

#define CGLTF_IMPLEMENTATION
#include <cgltf.h>

using namespace GEditor;

Model Model::Import(const Gleam::Filesystem::path& path)
{
	std::string gltf_path = path.string();
	cgltf_options options = {};
	cgltf_data* data = nullptr;

	cgltf_result result = cgltf_parse_file(&options, gltf_path.c_str(), &data);
	if (result != cgltf_result_success)
	{
		return {};
	}

	result = cgltf_load_buffers(&options, data, gltf_path.c_str());
	if (result != cgltf_result_success)
	{
		return {};
	}

	Model model;
	for (uint32_t i = 0; i < data->nodes_count; ++i)
	{
        cgltf_node& node = data->nodes[i];
        if (!node.mesh)
        {
            continue;
        }
        
		cgltf_mesh* mesh = node.mesh;
		for (uint32_t j = 0; j < mesh->primitives_count; ++j)
		{
			const auto& primitive = mesh->primitives[j];
			uint32_t vertex_count = primitive.attributes_count ? static_cast<uint32_t>(primitive.attributes[0].data->count) : 0u;

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
				for (uint32_t idx = 0; idx < vertex_count; ++idx)
				{
					meshData.indices[idx] = idx;
				}
			}

			// process attributes
			for (uint32_t ai = 0; ai < primitive.attributes_count; ++ai)
			{
				const auto& attribute = primitive.attributes[ai];
				if (attribute.type == cgltf_attribute_type_position)
				{
					meshData.positions.resize(vertex_count);
					cgltf_accessor_unpack_floats(attribute.data, (cgltf_float*)meshData.positions.data(), meshData.positions.size() * 3);
                    
                    if (node.has_translation)
                    {
                        for (auto& position : meshData.positions)
                        {
                            position.x += node.translation[0];
                            position.y += node.translation[1];
                            position.z += node.translation[2];
                        }
                    }
				}
				else if (attribute.type == cgltf_attribute_type_normal)
				{
					meshData.normals.resize(vertex_count);
					cgltf_accessor_unpack_floats(attribute.data, (cgltf_float*)meshData.normals.data(), meshData.normals.size() * 3);
				}
				else if (attribute.type == cgltf_attribute_type_texcoord)
				{
					meshData.texCoords.resize(vertex_count);
					cgltf_accessor_unpack_floats(attribute.data, (cgltf_float*)meshData.texCoords.data(), meshData.texCoords.size() * 2);
				}
			}

			if (meshData.texCoords.empty())
			{
				meshData.texCoords.resize(vertex_count, Gleam::Vector2::zero);
			}

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
