#pragma once

namespace Gleam {

class Mesh;
class MaterialInstance;

class MeshRenderer
{
public:

	MeshRenderer(const RefCounted<Mesh>& mesh);

    void SetMaterial(const RefCounted<MaterialInstance>& material, uint32_t index);

    const RefCounted<MaterialInstance>& GetMaterial(uint32_t index) const;

    const RefCounted<Mesh>& GetMesh() const;
    
private:
    
    RefCounted<Mesh> mMesh;
    
    TArray<RefCounted<MaterialInstance>> mMaterials;
    
};

} // namespace Gleam
