//
//  WorldRenderer.cpp
//  Runtime
//
//  Created by Batuhan Bozyel on 20.10.2022.
//

#include "gpch.h"
#include "WorldRenderer.h"
#include "CommandBuffer.h"

using namespace Gleam;

void WorldRenderer::AddRenderPasses(RenderGraph& graph, const RenderingData& renderData)
{
    struct BufferUpdatesPass
    {
        
    };
    
    graph.AddRenderPass<BufferUpdatesPass>("WorldRenderer::BufferUpdatesPass", [&](RenderGraphBuilder& builder, BufferUpdatesPass& passData)
    {
        
    }, [this, &renderData](const RenderGraphContext& context, const BufferUpdatesPass& passData)
    {
        CameraUniforms uniforms;
        uniforms.viewMatrix = mViewMatrix;
        uniforms.projectionMatrix = mProjectionMatrix;
        uniforms.viewProjectionMatrix = mViewProjectionMatrix;
        context.cmd->SetPushConstant(uniforms, ShaderStage_Vertex | ShaderStage_Fragment);
    });
    
}

void WorldRenderer::UpdateCamera(Camera& camera)
{
    mViewMatrix = camera.GetViewMatrix();
    mProjectionMatrix = camera.GetProjectionMatrix();
    mViewProjectionMatrix = mProjectionMatrix * mViewMatrix;
}
