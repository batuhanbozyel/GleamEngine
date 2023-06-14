#pragma once
#include "Model.h"
#include "Renderer/Mesh.h"
#include "Renderer/Material.h"

namespace Gleam {

class MeshRenderer
{
public:
    
    MeshRenderer(const Model& model)
        : mMesh(CreateRef<Mesh>(model.GetMeshes()))
    {
        
    }
    
    const RefCounted<Mesh>& GetMesh() const
    {
        return mMesh;
    }
    
    const RefCounted<Material>& GetMaterial() const
    {
        return mMaterial;
    }
    
private:
    
    RefCounted<Mesh> mMesh;
    
    RefCounted<Material> mMaterial;
    
};

} // namespace Gleam
