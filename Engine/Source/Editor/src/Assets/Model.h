#pragma once
#include "Gleam.h"

namespace GEditor {

class Model
{
public:
    
	static Model Import(const Gleam::Filesystem::path& path);

    Model(const Gleam::TArray<Gleam::MeshData>& meshes, const Gleam::TArray<Gleam::PBRMaterialData>& materials);

    const Gleam::TArray<Gleam::PBRMaterialData>& GetMaterials() const;
    
    const Gleam::TArray<Gleam::MeshData>& GetMeshes() const;

private:

    void CalculateNormalsIfNeeded();

    Gleam::TArray<Gleam::PBRMaterialData> mMaterials;
    
    Gleam::TArray<Gleam::MeshData> mMeshes;
};

} // namespace GEditor
