#include "ImGui/ImGuiBackend.h"

using namespace GEditor;

#ifdef USE_METAL_RENDERER
#include "Renderer/Metal/MetalUtils.h"

#include "backends/imgui_impl_sdl2.h"
#include "backends/imgui_impl_metal.h"

ImGuiBackend::ImGuiBackend()
{
    ImGui_ImplMetal_Init(Gleam::MetalDevice::GetHandle());
    ImGui_ImplSDL2_InitForMetal(GameInstance->GetWindow()->GetSDLWindow());
}

ImGuiBackend::~ImGuiBackend()
{
    ImGui_ImplMetal_Shutdown();
    ImGui_ImplSDL2_Shutdown();
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
    ImGui_ImplSDL2_NewFrame();
}

void ImGuiBackend::EndFrame(NativeGraphicsHandle commandBuffer, NativeGraphicsHandle renderCommandEncoder) const
{
    ImGui_ImplMetal_RenderDrawData(ImGui::GetDrawData(), commandBuffer, renderCommandEncoder);
}

#endif
