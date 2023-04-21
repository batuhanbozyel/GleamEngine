//
//  CompositeRenderer.h
//  Editor
//
//  Created by Batuhan Bozyel on 19.04.2023.
//

#pragma once
#include "Renderer/Renderer.h"

namespace Gleam {

class Shader;

class CompositeRenderer : public IRenderer
{
public:
    
    virtual void OnCreate(RendererContext* context) override;
    
    virtual void AddRenderPasses(RenderGraph& graph, RenderGraphBlackboard& blackboard) override;
    
private:
    
    RefCounted<Shader> mCompositePassVertexShader;
    RefCounted<Shader> mCompositePassFragmentShader;
    
    RendererContext* mRendererContext = nullptr;
    
};

} // namespace Gleam
