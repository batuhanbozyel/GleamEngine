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
	for (size_t i = 0; i < data->scenes_count; ++i)
	{
		cgltf_scene* scene = &data->scenes[i];
		for (size_t j = 0; j < scene->nodes_count; ++j)
		{
			cgltf_node* node = scene->nodes[j];
		}
	}
	cgltf_free(data);

	model.CalculateNormalsIfNeeded();
	return model;
}

void Model::CalculateNormalsIfNeeded()
{
    for (auto& mesh : mMeshes)
    {
        if (mesh.normals.size() == 0)
        {
            mesh.normals.resize(mesh.positions.size());
            for (uint32_t i = 0; i < mesh.indices.size(); i += 3)
            {
                auto& position1 = mesh.positions[mesh.indices[i + 0]];
                auto& position2 = mesh.positions[mesh.indices[i + 1]];
                auto& position3 = mesh.positions[mesh.indices[i + 2]];

                Gleam::Vector3 normal{ 0.0f, 0.0f, 0.0f };
                {
                    auto pos1 = position2 - position1;
                    auto pos2 = position3 - position1;
                    normal += Gleam::Math::Cross(pos1, pos2);
                }

                {
                    auto pos1 = position1 - position2;
                    auto pos2 = position3 - position2;
                    normal += Gleam::Math::Cross(pos1, pos2);
                }

                {
                    auto pos1 = position1 - position3;
                    auto pos2 = position2 - position3;
                    normal += Gleam::Math::Cross(pos1, pos2);
                }

                Gleam::Vector3 normalVec = Gleam::Math::Normalize(Gleam::Vector3{ normal.x, normal.y, normal.z });
                normal.x = normalVec.x;
                normal.y = normalVec.y;
                normal.z = normalVec.z;

                mesh.normals[mesh.indices[i + 0]] = normal;
                mesh.normals[mesh.indices[i + 1]] = normal;
                mesh.normals[mesh.indices[i + 2]] = normal;
            }
        }
    }
}

const Gleam::TArray<Gleam::MeshData>& Model::GetMeshes() const
{
    return mMeshes;
}
