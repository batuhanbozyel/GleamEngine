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
    
    virtual void OnCreate(GraphicsDevice* device) override;
    
    virtual void AddRenderPasses(RenderGraph& graph, RenderGraphBlackboard& blackboard) override;
    
private:
    
    Shader mFullscreenTriangleVertexShader;
    Shader mTonemappingFragmentShader;
    
};

} // namespace Gleam
