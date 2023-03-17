//
//  WorldRenderer.h
//  Runtime
//
//  Created by Batuhan Bozyel on 20.10.2022.
//

#pragma once
#include "Renderer/Renderer.h"

namespace Gleam {

class WorldRenderer : public IRenderer
{
public:

    virtual void AddRenderPasses(RenderGraph& graph, const RenderingData& renderData) override;

	void UpdateCamera(Camera& camera);

private:

    Matrix4 mViewMatrix;
    Matrix4 mProjectionMatrix;
    Matrix4 mViewProjectionMatrix;

};

} // namespace Gleam
