#include "ImGui/ImGuiBackend.h"

using namespace GEditor;

#ifdef USE_METAL_RENDERER
#include "Renderer/Metal/MetalUtils.h"

#include "backends/imgui_impl_sdl3.h"
#include "backends/imgui_impl_metal.h"

ImGuiBackend::ImGuiBackend()
{
    ImGui_ImplMetal_Init(Gleam::MetalDevice::GetHandle());
    ImGui_ImplSDL3_InitForMetal(GameInstance->GetSubsystem<Gleam::WindowSystem>()->GetSDLWindow());
}

ImGuiBackend::~ImGuiBackend()
{
    ImGui_ImplMetal_Shutdown();
    ImGui_ImplSDL3_Shutdown();
}

void ImGuiBackend::BeginFrame() const
{
    MTLRenderPassDescriptor* renderPassDesc = [MTLRenderPassDescriptor new];
    MTLRenderPassColorAttachmentDescriptor* colorAttachmentDesc = renderPassDesc.colorAttachments[0];
    colorAttachmentDesc.clearColor = { 0.0, 0.0, 0.0, 1.0 };
    colorAttachmentDesc.loadAction = MTLLoadActionLoad;
    colorAttachmentDesc.storeAction = MTLStoreActionStore;
    colorAttachmentDesc.texture = Gleam::MetalDevice::GetSwapchain().AcquireNextDrawable().texture;

    ImGui_ImplMetal_NewFrame(renderPassDesc);
    ImGui_ImplSDL3_NewFrame();
}

void ImGuiBackend::EndFrame(NativeGraphicsHandle commandBuffer, NativeGraphicsHandle renderCommandEncoder) const
{
    ImGui_ImplMetal_RenderDrawData(ImGui::GetDrawData(), commandBuffer, renderCommandEncoder);
}

#endif
