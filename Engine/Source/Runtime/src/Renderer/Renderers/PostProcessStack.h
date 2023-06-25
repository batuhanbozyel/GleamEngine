//
//  PostProcessStack.h
//  Runtime
//
//  Created by Batuhan Bozyel on 19.05.2023.
//

#pragma once
#include "Renderer/Renderer.h"

namespace Gleam {

class Shader;

class PostProcessStack : public IRenderer
{
public:
    
    virtual void OnCreate(RendererContext& context) override;
    
    virtual void AddRenderPasses(RenderGraph& graph, RenderGraphBlackboard& blackboard) override;
    
private:
    
    RefCounted<Shader> mFullscreenTriangleVertexShader;
    RefCounted<Shader> mTonemappingFragmentShader;
    
};

} // namespace Gleam
