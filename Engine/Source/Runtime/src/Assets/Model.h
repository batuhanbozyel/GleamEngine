#pragma once

namespace Gleam {

struct MaterialData
{
    TString baseTexture;
};

struct MeshData
{
    TArray<Vector3> positions;
    TArray<Vector3> normals;
    TArray<Vector2> texCoords;
    TArray<uint32_t> indices;
    MaterialData material;
};

class Model
{
public:
    
    Model(const TArray<MeshData>& meshes);

	static Model Import(const Filesystem::path& path);

    const TArray<MeshData>& GetMeshes() const;

private:

    void CalculateNormalsIfNeeded();

    TArray<MeshData> mMeshes;
};

} // namespace Gleam
