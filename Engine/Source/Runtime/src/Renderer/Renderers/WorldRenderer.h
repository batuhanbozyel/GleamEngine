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

class WorldRenderer : public IRenderer
{
public:
    
    struct FinalPassData
    {
        RenderTextureHandle colorTarget;
        RenderTextureHandle depthTarget;
    };
    
    void DrawMesh(const MeshRenderer& meshRenderer, const Matrix4& transform);
    
	void UpdateCamera(const Camera& camera);
    
    virtual void OnCreate(RendererContext* context) override;
    
    virtual void AddRenderPasses(RenderGraph& graph, RenderGraphBlackboard& blackboard) override;

private:
    
    Scope<Buffer> mCameraBuffer;
    RefCounted<Shader> mForwardPassVertexShader;
    RefCounted<Shader> mForwardPassFragmentShader;
    
    RendererContext* mRendererContext = nullptr;

};

} // namespace Gleam
