#pragma once
#ifdef USE_DIRECTX_RENDERER
#include "Container/String.h"
#include "Container/Queue.h"
#include "Container/Hash.h"

#include <d3d12.h>

namespace Gleam {

class DirectXDevice;
class DirectXCommandQueue;

struct DirectXCommandPool
{
	ID3D12Device10* mDevice = nullptr;
	DirectXCommandQueue* mQueue = nullptr;
	ID3D12CommandAllocator* mAllocator = nullptr;
	Deque<ID3D12GraphicsCommandList7*> mUsedCommandLists = {};
	Deque<ID3D12GraphicsCommandList7*> mFreeCommandLists = {};
	D3D12_COMMAND_LIST_TYPE mType = D3D12_COMMAND_LIST_TYPE_DIRECT;
	uint64_t mFenceValue = 0;

	ID3D12GraphicsCommandList7* AllocateCommandList(const TWStringView debugName);

	void Reset();
	void Release();
};

class DirectXCommandQueue final
{
	friend class DirectXDevice;
	friend struct DirectXCommandPool;
public:

	void Release();

	DirectXCommandPool* AcquirePool();

	void ExecuteCommandLists(UINT count, ID3D12GraphicsCommandList7** cmdLists);

	void Signal(ID3D12Fence* fence, uint64_t value) const;

	ID3D12CommandQueue* GetHandle() const;

private:

	uint64_t mFenceValue = 0;
	ID3D12Fence* mFence = nullptr;
	ID3D12Device10* mDevice = nullptr;
	ID3D12CommandQueue* mHandle = nullptr;
	Deque<DirectXCommandPool> mPools = {};
	HashMap<ID3D12GraphicsCommandList7*, DirectXCommandPool*> mCommandListToPoolMap = {};
	D3D12_COMMAND_LIST_TYPE mType = D3D12_COMMAND_LIST_TYPE_DIRECT;
};

} // namespace Gleam
#endif // USE_DIRECTX_RENDERER
