#include "ImGui/ImGuiBackend.h"

using namespace GEditor;

#ifdef USE_DIRECTX_RENDERER
#include "Renderer/DirectX/DirectXUtils.h"
#include "Renderer/DirectX/DirectXDevice.h"
#include "Renderer/DirectX/DirectXPipelineStateManager.h"

#include "ImGui/imgui_impl_sdl3.h"
#include "imgui_impl_dx12.h"

static ID3D12DescriptorHeap* gCbvSrvHeap = nullptr;

void ImGuiBackend::Init(Gleam::GraphicsDevice* device)
{
	mDevice = device;

	D3D12_DESCRIPTOR_HEAP_DESC desc{};
	desc.NumDescriptors = 1;
	desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	DX_CHECK(static_cast<ID3D12Device10*>(mDevice->GetHandle())->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&gCbvSrvHeap)));

	ImGui_ImplSDL3_InitForD3D(GameInstance->GetSubsystem<Gleam::WindowSystem>()->GetSDLWindow());
	ImGui_ImplDX12_Init(static_cast<ID3D12Device*>(mDevice->GetHandle()),
		mDevice->GetFramesInFlight(),
		Gleam::TextureFormatToDXGI_FORMAT(mDevice->GetFormat()),
		gCbvSrvHeap,
		gCbvSrvHeap->GetCPUDescriptorHandleForHeapStart(),
		gCbvSrvHeap->GetGPUDescriptorHandleForHeapStart());
}

void ImGuiBackend::Destroy()
{
	gCbvSrvHeap->Release();
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
	static_cast<ID3D12GraphicsCommandList7*>(commandBuffer)->SetDescriptorHeaps(1, &gCbvSrvHeap);
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), static_cast<ID3D12GraphicsCommandList*>(commandBuffer));
}

ImTextureID ImGuiBackend::GetImTextureIDForTexture(const Gleam::Texture& texture)
{
	return (ImTextureID)texture.GetHandle();
}

#endif
