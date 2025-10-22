#pragma once
#ifdef USE_DIRECTX_RENDERER
#include "Renderer/GraphicsDevice.h"
#include "Container/Queue.h"

#include <d3d12.h>
#include <dxgi1_6.h>

#ifdef GDEBUG
#include <d3d12sdklayers.h>
#include <dxgidebug.h>
#endif

namespace Gleam {

class DirectXSwapchain;

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

	D3D12_CPU_DESCRIPTOR_HANDLE Allocate();

	void Release(D3D12_CPU_DESCRIPTOR_HANDLE handle);

	ShaderResourceIndex GetResourceIndex(D3D12_CPU_DESCRIPTOR_HANDLE view);
};

struct DirectXCommandPool
{
	Deque<ID3D12GraphicsCommandList7*> usedCommandLists;
	Deque<ID3D12GraphicsCommandList7*> freeCommandLists;
	ID3D12CommandAllocator* allocator;
	D3D12_COMMAND_LIST_TYPE type;

	void Reset();
	void Release();
};

class DirectXDevice final : public GraphicsDevice
{
	friend class GraphicsDevice;

public:

	DirectXDevice(RenderSurface* surface, ResourceReleaseQueue* releaseQueue);

    ~DirectXDevice();

	DirectXDescriptorHeap& GetRtvHeap();

	DirectXDescriptorHeap& GetDsvHeap();

	DirectXDescriptorHeap& GetCbvSrvUavHeap();

	ID3D12GraphicsCommandList7* AllocateCommandList(D3D12_COMMAND_LIST_TYPE type, const TWStringView debugName);

	const DirectXDescriptorHeap& GetRtvHeap() const;

	const DirectXDescriptorHeap& GetDsvHeap() const;

	const DirectXDescriptorHeap& GetCbvSrvUavHeap() const;

	ID3D12CommandQueue* GetDirectQueue() const;

	ID3D12CommandQueue* GetComputeQueue() const;

	ID3D12CommandQueue* GetCopyQueue() const;

	ID3D12RootSignature* GetGlobalRootSignature() const;

	void WaitDeviceIdle() const;

	void WaitQueueIdle(ID3D12CommandQueue* queue) const;

	virtual void Configure(const RendererConfig& config) override;

	virtual ShaderResourceIndex CreateResourceView(const Buffer& buffer) override;

	virtual ShaderResourceIndex CreateResourceView(const Texture& texture) override;

	virtual void ReleaseResourceView(ShaderResourceIndex view) override;

	virtual void ResetCommandPools(uint32_t frameIdx) override;

private:

	ID3D12CommandQueue* CreateCommandQueue(D3D12_COMMAND_LIST_TYPE type) const;

	DirectXDescriptorHeap CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type, D3D12_DESCRIPTOR_HEAP_FLAGS flags, UINT capacity) const;

#ifdef GDEBUG
	DWORD mDebugCallbackCookie = 0;
	ID3D12InfoQueue1* mInfoQueue = nullptr;
	ID3D12Debug6* mD3D12Debug = nullptr;
#endif

	ID3D12CommandQueue* mDirectQueue = nullptr;

	ID3D12CommandQueue* mComputeQueue = nullptr;

	ID3D12CommandQueue* mCopyQueue = nullptr;

	ID3D12RootSignature* mRootSignature = nullptr;

	struct Context
	{
		TArray<DirectXCommandPool> commandPools;
	};
	TArray<Context> mFrameContext;

	DirectXDescriptorHeap mRtvHeap;
	DirectXDescriptorHeap mDsvHeap;
	DirectXDescriptorHeap mCbvSrvUavHeap;

};

} // namespace Gleam
#endif
