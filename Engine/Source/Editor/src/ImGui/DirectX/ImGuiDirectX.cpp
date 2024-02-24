#include "ImGui/ImGuiBackend.h"

using namespace GEditor;

#ifdef USE_DIRECTX_RENDERER
#include "Renderer/DirectX/DirectXDevice.h"
#include "Renderer/DirectX/DirectXPipelineStateManager.h"

#include "ImGui/imgui_impl_sdl3.h"
#include "imgui_impl_dx12.h"

void ImGuiBackend::Init(Gleam::GraphicsDevice* device)
{
	mDevice = device;

	ImGui_ImplSDL3_InitForD3D(GameInstance->GetSubsystem<Gleam::WindowSystem>()->GetSDLWindow());
	ImGui_ImplDX12_Init(&init_info, gRenderPass);
}

void ImGuiBackend::Destroy()
{
	ImGui_ImplDX12_Shutdown();
    ImGui_ImplSDL3_Shutdown();
}

void ImGuiBackend::BeginFrame()
{
	ImGui_ImplDX12_NewFrame();
    ImGui_ImplSDL3_NewFrame();
}

void ImGuiBackend::EndFrame(NativeGraphicsHandle commandBuffer, NativeGraphicsHandle renderPass)
{
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), static_cast<ID3D12GraphicsCommandList*>(commandBuffer));
}

ImTextureID ImGuiBackend::GetImTextureIDForTexture(const Gleam::Texture& texture)
{
	return (ImTextureID)texture.GetView();
}

#endif
