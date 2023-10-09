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
class Shader;
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
    
    virtual void OnCreate(RendererContext& context) override;
    
    virtual void AddRenderPasses(RenderGraph& graph, RenderGraphBlackboard& blackboard) override;
    
    void DrawMesh(const MeshRenderer& meshRenderer, const Transform& transform);
    
	void UpdateCamera(const Camera& camera);

private:
    
    struct RenderQueueElement
    {
        const Mesh* mesh;
        const MaterialInstance* material;
        Matrix4 transform;
    };
    HashMap<RefCounted<Material>, TArray<RenderQueueElement>> mOpaqueQueue;
    HashMap<RefCounted<Material>, TArray<RenderQueueElement>> mTransparentQueue;
    
    CameraUniforms mCameraData;
    RefCounted<Shader> mForwardPassVertexShader;
    RefCounted<Shader> mForwardPassFragmentShader;

};

} // namespace Gleam
