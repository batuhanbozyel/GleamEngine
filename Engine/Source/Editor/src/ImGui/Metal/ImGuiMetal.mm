#include "ImGui/ImGuiBackend.h"

using namespace GEditor;

#ifdef USE_METAL_RENDERER
#include "Renderer/Metal/MetalUtils.h"
#include "Renderer/Metal/MetalDevice.h"

#include "ImGui/imgui_impl_sdl3.h"
#include "imgui_impl_metal.h"

void ImGuiBackend::Init(Gleam::GraphicsDevice* device)
{
    mDevice = device;
    ImGui_ImplMetal_Init(static_cast<Gleam::MetalDevice*>(mDevice)->GetHandle());
    ImGui_ImplSDL3_InitForMetal(GameInstance->GetSubsystem<Gleam::WindowSystem>()->GetSDLWindow());
}

void ImGuiBackend::Destroy()
{
    ImGui_ImplMetal_Shutdown();
    ImGui_ImplSDL3_Shutdown();
}

void ImGuiBackend::BeginFrame()
{
    MTLRenderPassDescriptor* renderPassDesc = [MTLRenderPassDescriptor new];
    MTLRenderPassColorAttachmentDescriptor* colorAttachmentDesc = renderPassDesc.colorAttachments[0];
    colorAttachmentDesc.clearColor = { 0.0, 0.0, 0.0, 1.0 };
    colorAttachmentDesc.loadAction = MTLLoadActionLoad;
    colorAttachmentDesc.storeAction = MTLStoreActionStore;
    colorAttachmentDesc.texture = static_cast<Gleam::MetalDevice*>(mDevice)->AcquireNextDrawable().texture;

    ImGui_ImplMetal_NewFrame(renderPassDesc);
    ImGui_ImplSDL3_NewFrame();
}

void ImGuiBackend::EndFrame(NativeGraphicsHandle commandBuffer, NativeGraphicsHandle renderCommandEncoder)
{
    ImGui_ImplMetal_RenderDrawData(ImGui::GetDrawData(), commandBuffer, renderCommandEncoder);
}

ImTextureID ImGuiBackend::GetImTextureIDForTexture(const Gleam::Texture& texture)
{
    return (__bridge ImTextureID)texture.GetHandle();
}

#endif
