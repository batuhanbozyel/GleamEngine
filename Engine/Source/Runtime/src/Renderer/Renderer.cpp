#include "gpch.h"
#include "Renderer.h"
#include "RendererContext.h"
#include "ShaderLibrary.h"

using namespace Gleam;

void Renderer::Init(const TString& appName, const Version& appVersion, const RendererProperties& props)
{
    RendererContext::Init(appName, appVersion, props);
    ShaderLibrary::Init();
}

void Renderer::Destroy()
{
	ShaderLibrary::Destroy();
    RendererContext::Destroy();
}

const Size& Renderer::GetDrawableSize()
{
    return RendererContext::GetProperties().size;
}

Renderer::Renderer()
    : mCameraBuffers(RendererContext::GetSwapchain()->GetProperties().maxFramesInFlight)
{
    Camera defaultCamera(Renderer::GetDrawableSize());
    const auto& viewMatrix = defaultCamera.GetViewMatrix();
    const auto& projectionMatrix = defaultCamera.GetProjectionMatrix();
    
    CameraUniforms uniforms;
    uniforms.viewMatrix = viewMatrix;
    uniforms.projectionMatrix = projectionMatrix;
    uniforms.viewProjectionMatrix = projectionMatrix * viewMatrix;
    TArray<CameraUniforms, 1> cameraContainer = { uniforms };
    
    for (uint32_t i = 0; i < RendererContext::GetSwapchain()->GetProperties().maxFramesInFlight; i++)
    {
        mCameraBuffers[i] = std::move(CreateScope<UniformBuffer<CameraUniforms, MemoryType::Dynamic>>(cameraContainer));
    }
}

void Renderer::UpdateView(Camera& camera)
{
    const auto& viewMatrix = camera.GetViewMatrix();
    const auto& projectionMatrix = camera.GetProjectionMatrix();
    
    CameraUniforms uniforms;
    uniforms.viewMatrix = viewMatrix;
    uniforms.projectionMatrix = projectionMatrix;
    uniforms.viewProjectionMatrix = projectionMatrix * viewMatrix;
    GetCameraBuffer().SetData(uniforms);
}

const UniformBuffer<CameraUniforms, MemoryType::Dynamic>& Renderer::GetCameraBuffer() const
{
    auto frameIdx = RendererContext::GetSwapchain()->GetFrameIndex();
    return *mCameraBuffers[frameIdx];
}
