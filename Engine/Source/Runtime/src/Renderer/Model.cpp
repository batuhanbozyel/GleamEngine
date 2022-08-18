#include "gpch.h"
#include "Model.h"

using namespace Gleam;

Model::Model(const TArray<MeshData>& meshes)
	: mMeshes(meshes)
{
	CalculateNormals();
}

void Model::CalculateNormals()
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

				Vector3 normal{ 0.0f, 0.0f, 0.0f };
				{
					Vector3 pos1 = position2 - position1;
					Vector3 pos2 = position3 - position1;
					normal += Math::Cross(pos1, pos2);
				}

				{
					Vector3 pos1 = position1 - position2;
					Vector3 pos2 = position3 - position2;
					normal += Math::Cross(pos1, pos2);
				}

				{
					Vector3 pos1 = position1 - position3;
					Vector3 pos2 = position2 - position3;
					normal += Math::Cross(pos1, pos2);
				}

				Vector3 normalVec = Math::Normalize(Vector3{ normal.x, normal.y, normal.z });
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

const TArray<MeshData>& Model::GetMeshes() const
{
    return mMeshes;
}
