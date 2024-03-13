#pragma once
#include "Gleam.h"

namespace GEditor {

class Model
{
public:

	/*
	* glTF file requirements:
	*	- position, normal, uv attributes
	*	- triangulated primitive type and indices
	*/
	static Model Import(const Gleam::Filesystem::path& path);
    
    const Gleam::TArray<Gleam::MeshData>& GetMeshes() const;

private:
    
    Gleam::TArray<Gleam::MeshData> mMeshes;
};

} // namespace GEditor
