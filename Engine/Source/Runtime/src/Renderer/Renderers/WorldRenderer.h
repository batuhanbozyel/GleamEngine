//
//  WorldRenderer.h
//  Runtime
//
//  Created by Batuhan Bozyel on 20.10.2022.
//

#pragma once
#include "Renderer/Renderer.h"

namespace Gleam {

class Mesh;
class Material;

struct WorldRenderingData
{
    TextureHandle colorTarget;
    TextureHandle depthTarget;
    BufferHandle cameraBuffer;
};

class WorldRenderer : public IRenderer
{
public:
    
    virtual void OnCreate(GraphicsDevice* device) override;
    
    virtual void AddRenderPasses(RenderGraph& graph, RenderGraphBlackboard& blackboard) override;
    
    void DrawMesh(const MeshRenderer& meshRenderer, const Transform& transform);

private:
    
    struct RenderProxy
    {
        const Mesh* mesh;
        const MaterialInstance* material;
        Float4x4 transform;
    };
    HashMap<RefCounted<Material>, TArray<RenderProxy>> mOpaqueQueue;
    HashMap<RefCounted<Material>, TArray<RenderProxy>> mTransparentQueue;
    
    Shader mForwardPassVertexShader;
    Shader mForwardPassFragmentShader;

};

} // namespace Gleam
