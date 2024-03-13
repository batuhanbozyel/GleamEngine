#include "ImGui/ImGuiBackend.h"

using namespace GEditor;

#ifdef USE_DIRECTX_RENDERER
#include "Renderer/DirectX/DirectXUtils.h"
#include "Renderer/DirectX/DirectXDevice.h"
#include "Renderer/DirectX/DirectXPipelineStateManager.h"

#include "ImGui/imgui_impl_sdl3.h"
#include "imgui_impl_dx12.h"

void ImGuiBackend::Init(Gleam::GraphicsDevice* device)
{
	mDevice = device;
	auto& cbvSrvUavHeap = static_cast<Gleam::DirectXDevice*>(mDevice)->GetCbvSrvUavHeap();
	auto index = cbvSrvUavHeap.heap.Allocate();

	D3D12_CPU_DESCRIPTOR_HANDLE fontSrvCPUdescriptor = cbvSrvUavHeap.handle->GetCPUDescriptorHandleForHeapStart();
	fontSrvCPUdescriptor.ptr += (UINT64)(index.data * cbvSrvUavHeap.size);
	D3D12_GPU_DESCRIPTOR_HANDLE fontSrvGPUdescriptor = cbvSrvUavHeap.handle->GetGPUDescriptorHandleForHeapStart();
	fontSrvGPUdescriptor.ptr += (UINT64)(index.data * cbvSrvUavHeap.size);

	ImGui_ImplSDL3_InitForD3D(GameInstance->GetSubsystem<Gleam::WindowSystem>()->GetSDLWindow());
	ImGui_ImplDX12_Init(static_cast<ID3D12Device*>(mDevice->GetHandle()),
		mDevice->GetFramesInFlight(),
		Gleam::TextureFormatToDXGI_FORMAT(mDevice->GetFormat()),
		cbvSrvUavHeap.handle,
		fontSrvCPUdescriptor,
		fontSrvGPUdescriptor);
}

void ImGuiBackend::Destroy()
{
	static_cast<Gleam::DirectXDevice*>(mDevice)->WaitDeviceIdle();
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
	auto& cbvSrvUavHeap = static_cast<Gleam::DirectXDevice*>(mDevice)->GetCbvSrvUavHeap();
	D3D12_GPU_DESCRIPTOR_HANDLE srvGPUdescriptor = cbvSrvUavHeap.handle->GetGPUDescriptorHandleForHeapStart();
	srvGPUdescriptor.ptr += (UINT64)(texture.GetResourceView().data * cbvSrvUavHeap.size);
	return (ImTextureID)srvGPUdescriptor.ptr;
}

#endif
