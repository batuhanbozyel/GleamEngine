#include "../ImGui/ImGuiBackend.h"

#ifdef USE_METAL_RENDERER
#include "MetalUtils.h"
#include "MetalDevice.h"

#include "imgui_impl_metal.h"
#include "Core/Engine.h"
#include "Core/Globals.h"
#include "Core/WindowSystem.h"

#include "Renderer/Swapchain.h"
#include "Renderer/ImGui/imgui_impl_sdl3.h"

using namespace Gleam;

void ImGuiBackend::Init(RenderContext& context)
{
    mDevice = context.device;
    mSurface = context.surface;
    
    ImGui_ImplMetal_Init(static_cast<MetalDevice*>(mDevice)->GetHandle());
    ImGui_ImplSDL3_InitForMetal(Globals::Engine->GetSubsystem<WindowSystem>()->GetSDLWindow());
}

void ImGuiBackend::Destroy()
{
    ImGui_ImplMetal_Shutdown();
    ImGui_ImplSDL3_Shutdown();
}

void ImGuiBackend::BeginFrame()
{
    id<CAMetalDrawable> drawable = static_cast<Swapchain*>(mSurface)->AcquireNextDrawable().GetHandle();
    
    MTLRenderPassDescriptor* renderPassDesc = [MTLRenderPassDescriptor new];
    MTLRenderPassColorAttachmentDescriptor* colorAttachmentDesc = renderPassDesc.colorAttachments[0];
    colorAttachmentDesc.clearColor = { 0.0, 0.0, 0.0, 1.0 };
    colorAttachmentDesc.loadAction = MTLLoadActionLoad;
    colorAttachmentDesc.storeAction = MTLStoreActionStore;
    colorAttachmentDesc.texture = drawable.texture;

    ImGui_ImplMetal_NewFrame(renderPassDesc);
    ImGui_ImplSDL3_NewFrame();
}

void ImGuiBackend::EndFrame(NativeGraphicsHandle commandBuffer, NativeGraphicsHandle renderCommandEncoder)
{
    ImGui_ImplMetal_RenderDrawData(ImGui::GetDrawData(), commandBuffer, renderCommandEncoder);
}

ImTextureID ImGuiBackend::GetImTextureIDForTexture(const Texture& texture)
{
    return (__bridge ImTextureID)texture.GetHandle();
}

#endif
