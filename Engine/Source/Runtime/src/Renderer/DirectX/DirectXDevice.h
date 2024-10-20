#pragma once
#ifdef USE_DIRECTX_RENDERER
#include "Renderer/GraphicsDevice.h"

#include <d3d12.h>
#include <dxgi1_6.h>

#ifdef GDEBUG
#include <d3d12sdklayers.h>
#include <dxgidebug.h>
#endif

namespace Gleam {

class DirectXDescriptorHeap
{
public:
	ResourceDescriptorHeap heap;
	ID3D12DescriptorHeap* handle = nullptr;
	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle;
	D3D12_DESCRIPTOR_HEAP_TYPE type;
	UINT size;
	UINT capacity;

	ShaderResourceIndex GetResourceIndex(D3D12_CPU_DESCRIPTOR_HANDLE view);
};

struct DirectXCommandList
{
	ID3D12GraphicsCommandList7* handle;
	ID3D12CommandAllocator* allocator;
};

struct DirectXCommandPool
{
	Deque<DirectXCommandList> usedCommandLists;
	Deque<DirectXCommandList> freeCommandLists;

	void Reset();

	void Release();
};

struct DirectXDrawable
{
	ID3D12Resource* renderTarget;
	D3D12_CPU_DESCRIPTOR_HANDLE view;
};

class DirectXDevice final : public GraphicsDevice
{
	friend class GraphicsDevice;

public:

	DirectXDevice();

    ~DirectXDevice();

	DirectXDrawable AcquireNextDrawable();

	DirectXDescriptorHeap& GetCbvSrvUavHeap();

	ID3D12GraphicsCommandList7* AllocateCommandList(D3D12_COMMAND_LIST_TYPE type);

	const DirectXDescriptorHeap& GetCbvSrvUavHeap() const;

	ID3D12CommandQueue* GetDirectQueue() const;

	ID3D12CommandQueue* GetComputeQueue() const;

	ID3D12CommandQueue* GetCopyQueue() const;

	void WaitDeviceIdle() const;

	void WaitQueueIdle(ID3D12CommandQueue* queue) const;

	virtual ShaderResourceIndex CreateResourceView(const Buffer& buffer) override;

	virtual ShaderResourceIndex CreateResourceView(const Texture& texture) override;

	virtual void ReleaseResourceView(ShaderResourceIndex view) override;

private:

	virtual void Present(const CommandBuffer* cmd) override;

	virtual void Configure(const RendererConfig& config) override;

	virtual void DestroyFrameObjects(uint32_t frameIndex) override;

	ID3D12CommandQueue* CreateCommandQueue(D3D12_COMMAND_LIST_TYPE type) const;

	DirectXDescriptorHeap CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type, D3D12_DESCRIPTOR_HEAP_FLAGS flags, UINT capacity) const;

	DirectXDrawable GetSwapchainBuffer(uint32_t buffer);

	void ReleaseSwapchainBuffer(DirectXDrawable& drawable);

#ifdef GDEBUG
	DWORD mDebugCallbackCookie = 0;
	ID3D12InfoQueue1* mInfoQueue = nullptr;

	ID3D12Debug6* mD3D12Debug = nullptr;
	IDXGIDebug1* mDXGIDebug = nullptr;
#endif

	IDXGISwapChain4* mSwapchain = nullptr;

	IDXGIAdapter4* mAdapter = nullptr;

	IDXGIFactory7* mFactory = nullptr;

	ID3D12Fence* mDirectFence = nullptr;

	ID3D12CommandQueue* mDirectQueue;

	ID3D12CommandQueue* mComputeQueue;

	ID3D12CommandQueue* mCopyQueue;

	struct Context
	{
		ID3D12Fence* fence;
		DirectXDrawable drawable;
		DirectXCommandPool commandPool;
		uint32_t frameCount = 0;
	};
	TArray<Context> mFrameContext;

	DirectXDescriptorHeap mRtvHeap;
	DirectXDescriptorHeap mDsvHeap;
	DirectXDescriptorHeap mCbvSrvUavHeap;

};

} // namespace Gleam
#endif
