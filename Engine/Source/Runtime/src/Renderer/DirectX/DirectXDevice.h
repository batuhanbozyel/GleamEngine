#pragma once
#ifdef USE_DIRECTX_RENDERER
#include "Renderer/GraphicsDevice.h"

#include <d3d12.h>
#include <dxcapi.h>
#include <dxgi1_6.h>
#include <D3D12MemAlloc.h>

namespace Gleam {

struct DirectXCommandList
{
	ID3D12GraphicsCommandList7* handle;
	ID3D12CommandAllocator* allocator;
};

struct DirectXDrawable
{
	ID3D12Resource* renderTarget;
	D3D12_CPU_DESCRIPTOR_HANDLE descriptor;
};

class DirectXDevice final : public GraphicsDevice
{
public:

	DirectXDevice();

    ~DirectXDevice();

	DirectXDrawable AcquireNextDrawable();
	void Present(ID3D12GraphicsCommandList7* commandList);

	ID3D12GraphicsCommandList7* AllocateCommandList(D3D12_COMMAND_LIST_TYPE type);

	ID3D12CommandQueue* GetDirectQueue() const;

	ID3D12CommandQueue* GetComputeQueue() const;

	ID3D12CommandQueue* GetCopyQueue() const;

	IDxcUtils* GetDxcUtils() const;

private:

	virtual void DestroyFrameObjects(uint32_t frameIndex) override;

	virtual void Configure(const RendererConfig& config) override;

	IDxcUtils* mDxcUtils = nullptr;

	IDXGISwapChain4* mSwapchain = nullptr;

	IDXGIAdapter4* mAdapter = nullptr;

	IDXGIFactory7* mFactory = nullptr;

	ID3D12CommandQueue* mDirectQueue = nullptr;

	ID3D12CommandQueue* mComputeQueue = nullptr;

	ID3D12CommandQueue* mCopyQueue = nullptr;

	ID3D12CommandAllocator* mDirectCommandAllocator = nullptr;

	ID3D12CommandAllocator* mComputeCommandAllocator = nullptr;

	ID3D12CommandAllocator* mCopyCommandAllocator = nullptr;

	// Frame
	struct FramePool
	{
		TArray<DirectXCommandList> commandLists;
	};
	TArray<FramePool> mFrameObjects;

	ID3D12DescriptorHeap* mRTVHeap = nullptr;
	TArray<ID3D12Resource*> mRenderTargets;

};

} // namespace Gleam
#endif
