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

struct WorldRenderingData
{
    RenderTextureHandle colorTarget;
    RenderTextureHandle depthTarget;
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
        RefCounted<Mesh> mesh;
        Matrix4 transform;
    };
    List<RenderQueueElement> mOpaqueQueue;
    
    Scope<Buffer> mCameraBuffer;
    RefCounted<Shader> mForwardPassVertexShader;
    RefCounted<Shader> mForwardPassFragmentShader;

};

} // namespace Gleam