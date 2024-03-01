#pragma once
#ifdef USE_DIRECTX_RENDERER
#include "Renderer/GraphicsDevice.h"

#include <d3d12.h>
#include <dxcapi.h>
#include <dxgi1_6.h>

namespace Gleam {

class DirectXDescriptorHeap
{
public:
	ID3D12DescriptorHeap* handle = nullptr;
	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle;
	D3D12_DESCRIPTOR_HEAP_TYPE type;
	UINT size;
	UINT capacity;
};

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

	DirectXCommandList AllocateCommandList(D3D12_COMMAND_LIST_TYPE type);

	const DirectXDescriptorHeap& GetCbvSrvUavHeap() const;

	ID3D12CommandQueue* GetDirectQueue() const;

	ID3D12CommandQueue* GetComputeQueue() const;

	ID3D12CommandQueue* GetCopyQueue() const;

	IDxcUtils* GetDxcUtils() const;

private:

	virtual void Present(const CommandBuffer* cmd) override;

	virtual void Configure(const RendererConfig& config) override;

	ID3D12CommandQueue* CreateCommandQueue(D3D12_COMMAND_LIST_TYPE type) const;

	DirectXDescriptorHeap CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type, D3D12_DESCRIPTOR_HEAP_FLAGS flags, UINT capacity) const;

	IDxcUtils* mDxcUtils = nullptr;

	IDXGISwapChain4* mSwapchain = nullptr;

	IDXGIAdapter4* mAdapter = nullptr;

	IDXGIFactory7* mFactory = nullptr;

	ID3D12Fence* mDirectFence = nullptr;

	ID3D12CommandQueue* mDirectQueue;

	ID3D12CommandQueue* mComputeQueue;

	ID3D12CommandQueue* mCopyQueue;

	struct CommandPool
	{
		Deque<DirectXCommandList> usedCommandLists;
		Deque<DirectXCommandList> freeCommandLists;

		void Reset();

		void Release();
	};

	struct Context
	{
		ID3D12Fence* fence;
		ID3D12Resource* renderTarget;
		CommandPool commandPool;
		uint32_t frameCount = 0;
	};
	TArray<Context> mFrameContext;

	DirectXDescriptorHeap mRtvHeap;
	DirectXDescriptorHeap mDsvHeap;
	DirectXDescriptorHeap mCbvSrvUavHeap;

};

} // namespace Gleam
#endif
