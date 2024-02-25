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

struct DirectXCommandQueue
{
	ID3D12Fence* fence;
	ID3D12CommandQueue* handle;
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

	DirectXCommandList AllocateCommandList(D3D12_COMMAND_LIST_TYPE type);

	ID3D12CommandQueue* GetDirectQueue() const;

	ID3D12CommandQueue* GetComputeQueue() const;

	ID3D12CommandQueue* GetCopyQueue() const;

	IDxcUtils* GetDxcUtils() const;

private:

	virtual void Configure(const RendererConfig& config) override;

	DirectXCommandQueue CreateCommandQueue(D3D12_COMMAND_LIST_TYPE type) const;

	IDxcUtils* mDxcUtils = nullptr;

	IDXGISwapChain4* mSwapchain = nullptr;

	IDXGIAdapter4* mAdapter = nullptr;

	IDXGIFactory7* mFactory = nullptr;

	ID3D12Fence* mDirectFence = nullptr;

	DirectXCommandQueue mDirectQueue;

	DirectXCommandQueue mComputeQueue;

	DirectXCommandQueue mCopyQueue;

	// Frame
	struct CommandPool
	{
		Deque<DirectXCommandList> usedCommandLists;
		Deque<DirectXCommandList> freeCommandLists;

		void Reset();

		void Release();
	};
	TArray<CommandPool> mCommandPools;

	ID3D12DescriptorHeap* mRTVHeap = nullptr;

	TArray<ID3D12Resource*> mRenderTargets;

};

} // namespace Gleam
#endif
