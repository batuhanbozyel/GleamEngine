//
//  PostProcessStack.h
//  Runtime
//
//  Created by Batuhan Bozyel on 19.05.2023.
//

#pragma once
#include "Renderer/Renderer.h"

namespace Gleam {

class PostProcessStack : public IRenderer
{
public:
    
    virtual void OnCreate(const RenderContext& context) override;
    
    virtual void AddRenderPasses(RenderGraph& graph, RenderGraphBlackboard& blackboard) override;

	virtual RenderStage GetStage() const override { return RenderStage::PostProcess; }

private:
    
	GraphicsPipelineHandle mPipeline;
    
};

} // namespace Gleam
