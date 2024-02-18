#pragma once
#include "Renderer/Renderer.h"

namespace GEditor {

class InfiniteGridRenderer final : public Gleam::IRenderer
{
public:

    virtual void OnCreate(Gleam::GraphicsDevice* device) override;
    
    virtual void AddRenderPasses(Gleam::RenderGraph& graph, Gleam::RenderGraphBlackboard& blackboard) override;

private:
    
    Gleam::Shader mVertexShader;
    Gleam::Shader mFragmentShader;

};

} // namespace Gleam

