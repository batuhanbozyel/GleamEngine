#pragma once
#include "Renderer/Mesh.h"
#include "Renderer/Material/MaterialInstance.h"

namespace Gleam {

class MeshRenderer
{
public:

    MeshRenderer(const Mesh& mesh);

    void SetMaterial(const MaterialInstance& material, uint32_t index);

    const MaterialInstance& GetMaterial(uint32_t index) const;

    const Mesh& GetMesh() const;
    
private:
    
    Mesh mMesh;
    
    TArray<MaterialInstance> mMaterials;
    
};

} // namespace Gleam
