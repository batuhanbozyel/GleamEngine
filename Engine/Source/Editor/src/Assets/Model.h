#pragma once
#include "Gleam.h"

namespace GEditor {

class Model
{
public:
    
	static Model Import(const Gleam::Filesystem::path& path);
    
    const Gleam::TArray<Gleam::MeshData>& GetMeshes() const;

private:

    void CalculateNormalsIfNeeded();
    
    Gleam::TArray<Gleam::MeshData> mMeshes;
};

} // namespace GEditor
