#include "gpch.h"

#ifdef USE_DIRECTX_RENDERER
#include "DirectXCommandQueue.h"
#include "DirectXUtils.h"

using namespace Gleam;

ID3D12GraphicsCommandList7* DirectXCommandPool::AllocateCommandList(const TWStringView debugName)
{
	ID3D12GraphicsCommandList7* commandList = nullptr;
	if (mFreeCommandLists.empty())
	{
		DX_CHECK(mDevice->CreateCommandList1(0, mType, D3D12_COMMAND_LIST_FLAG_NONE, IID_PPV_ARGS(&commandList)));
	}
	else
	{
		commandList = mFreeCommandLists.front();
		mFreeCommandLists.pop_front();
	}
	mQueue->mCommandListToPoolMap[commandList] = this;

	mUsedCommandLists.push_back(commandList);
	DX_CHECK(commandList->Reset(mAllocator, nullptr));
	commandList->SetName(debugName.data());
	return commandList;
}

void DirectXCommandPool::Reset()
{
	mFreeCommandLists.insert(mFreeCommandLists.end(), mUsedCommandLists.begin(), mUsedCommandLists.end());
	mUsedCommandLists.clear();
	DX_CHECK(mAllocator->Reset());
}

void DirectXCommandPool::Release()
{
	for (auto cmdList : mUsedCommandLists)
	{
		cmdList->Release();
	}
	mUsedCommandLists.clear();

	for (auto cmdList : mFreeCommandLists)
	{
		cmdList->Release();
	}
	mFreeCommandLists.clear();

	mAllocator->Release();
	mAllocator = nullptr;
}

void DirectXCommandQueue::Release()
{
	mCommandListToPoolMap.clear();
	for (auto& pool : mPools)
	{
		pool.Release();
	}
	mPools.clear();

	mFence->Release();
	mFence = nullptr;

	mHandle->Release();
	mHandle = nullptr;
}

DirectXCommandPool* DirectXCommandQueue::AcquirePool()
{
	const uint64_t completed = mFence->GetCompletedValue();
	for (auto& pool : mPools)
	{
		if (pool.mFenceValue <= completed)
		{
			pool.Reset();
			return &pool;
		}
	}

	auto& pool = mPools.emplace_back();
	pool.mDevice = mDevice;
	pool.mQueue = this;
	pool.mType = mType;
	DX_CHECK(mDevice->CreateCommandAllocator(mType, IID_PPV_ARGS(&pool.mAllocator)));
	return &pool;
}

void DirectXCommandQueue::ExecuteCommandLists(UINT count, ID3D12GraphicsCommandList7** cmdLists)
{
	mHandle->ExecuteCommandLists(count, reinterpret_cast<ID3D12CommandList* const*>(cmdLists));

	uint64_t fenceValue = ++mFenceValue;
	DX_CHECK(mHandle->Signal(mFence, fenceValue));

	for (UINT i = 0; i < count; ++i)
	{
		mCommandListToPoolMap[cmdLists[i]]->mFenceValue = fenceValue;
	}
}

void DirectXCommandQueue::Signal(ID3D12Fence* fence, uint64_t value) const
{
	DX_CHECK(mHandle->Signal(fence, value));
}

ID3D12CommandQueue* DirectXCommandQueue::GetHandle() const
{
	return mHandle;
}

#endif // USE_DIRECTX_RENDERER