#pragma once

namespace Gleam {

class Mesh;
class Material;
class MaterialInstance;

struct MeshBatch
{
	Mesh mesh;
    MaterialInstance material;
    Float4x4 transform;
};

class RenderSceneProxy
{
    using BatchFn = std::function<void(const Material*, const TArray<MeshBatch>&)>;
public:
    
    void Update(const World* world);
    
    void ForEach(BatchFn&& fn) const;
    
    const Camera* GetActiveCamera() const;

private:
    
    const Camera* mActiveCamera = nullptr;
    
    HashMap<const Material*, TArray<MeshBatch>> mStaticBatches;
    
};

} // namespace Gleam
