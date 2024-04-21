#include "../ImGui/ImGuiBackend.h"

#ifdef USE_DIRECTX_RENDERER
#include "DirectXUtils.h"
#include "DirectXDevice.h"
#include "DirectXPipelineStateManager.h"

#include "imgui_impl_dx12.h"
#include "Core/Application.h"
#include "Core/WindowSystem.h"
#include "Renderer/ImGui/imgui_impl_sdl3.h"

using namespace Gleam;

void ImGuiBackend::Init(GraphicsDevice* device)
{
	mDevice = device;
	auto& cbvSrvUavHeap = static_cast<DirectXDevice*>(mDevice)->GetCbvSrvUavHeap();
	auto index = cbvSrvUavHeap.heap.Allocate();

	D3D12_CPU_DESCRIPTOR_HANDLE fontSrvCPUdescriptor = cbvSrvUavHeap.handle->GetCPUDescriptorHandleForHeapStart();
	fontSrvCPUdescriptor.ptr += (UINT64)(index.data * cbvSrvUavHeap.size);
	D3D12_GPU_DESCRIPTOR_HANDLE fontSrvGPUdescriptor = cbvSrvUavHeap.handle->GetGPUDescriptorHandleForHeapStart();
	fontSrvGPUdescriptor.ptr += (UINT64)(index.data * cbvSrvUavHeap.size);

	ImGui_ImplSDL3_InitForD3D(Globals::Engine->GetSubsystem<WindowSystem>()->GetSDLWindow());
	ImGui_ImplDX12_Init(static_cast<ID3D12Device*>(mDevice->GetHandle()),
		mDevice->GetFramesInFlight(),
		TextureFormatToDXGI_FORMAT(mDevice->GetFormat()),
		cbvSrvUavHeap.handle,
		fontSrvCPUdescriptor,
		fontSrvGPUdescriptor);
}

void ImGuiBackend::Destroy()
{
	static_cast<DirectXDevice*>(mDevice)->WaitDeviceIdle();
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

ImTextureID ImGuiBackend::GetImTextureIDForTexture(const Texture& texture)
{
	auto& cbvSrvUavHeap = static_cast<DirectXDevice*>(mDevice)->GetCbvSrvUavHeap();
	D3D12_GPU_DESCRIPTOR_HANDLE srvGPUdescriptor = cbvSrvUavHeap.handle->GetGPUDescriptorHandleForHeapStart();
	srvGPUdescriptor.ptr += (UINT64)(texture.GetResourceView().data * cbvSrvUavHeap.size);
	return (ImTextureID)srvGPUdescriptor.ptr;
}

#endif
