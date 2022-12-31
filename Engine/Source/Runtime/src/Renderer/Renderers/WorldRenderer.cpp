//
//  WorldRenderer.cpp
//  Runtime
//
//  Created by Batuhan Bozyel on 20.10.2022.
//

#include "gpch.h"
#include "WorldRenderer.h"

using namespace Gleam;

WorldRenderer::WorldRenderer(RendererContext& context)
    : Renderer(context)
{
    Camera defaultCamera(context.GetDrawableSize());
    const auto& viewMatrix = defaultCamera.GetViewMatrix();
    const auto& projectionMatrix = defaultCamera.GetProjectionMatrix();
    
    CameraUniforms uniforms;
    uniforms.viewMatrix = viewMatrix;
    uniforms.projectionMatrix = projectionMatrix;
    uniforms.viewProjectionMatrix = projectionMatrix * viewMatrix;
	mCameraBuffer = CreateScope<UniformBuffer<CameraUniforms, MemoryType::Dynamic>>(uniforms);
}

void WorldRenderer::Execute()
{
	
}

void WorldRenderer::Update(Camera& camera)
{
    CameraUniforms uniforms;
    uniforms.viewMatrix = camera.GetViewMatrix();
    uniforms.projectionMatrix = camera.GetProjectionMatrix();
    uniforms.viewProjectionMatrix = projectionMatrix * viewMatrix;
    mCameraBuffer->SetData(uniforms);
}