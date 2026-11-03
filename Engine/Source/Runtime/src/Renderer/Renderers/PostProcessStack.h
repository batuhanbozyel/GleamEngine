//
//  PostProcessStack.h
//  Runtime
//
//  Created by Batuhan Bozyel on 19.05.2023.
//

#pragma once
#include "Renderer/Renderer.h"

namespace Gleam {

GCLASS(PostProcessStack, "5B2A9D04-2BA6-412B-81EB-370AB9404B38", Renderer(RenderStage::PostProcess), PrettyName("Post Process")) : public IRenderer
{
public:
    
    virtual void OnCreate(const RenderContext& context) override;
    
    virtual void AddRenderPasses(RenderGraph& graph, RenderGraphBlackboard& blackboard) override;

private:
    
	GraphicsPipelineHandle mPipeline;
    
};

} // namespace Gleam
