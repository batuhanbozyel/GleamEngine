#include "gpch.h"

#ifdef USE_DIRECTX_RENDERER
#include "DirectXTransitionManager.h"

using namespace Gleam;

void DirectXTransitionManager::Init(DirectXDevice* device)
{
	mDevice = device;
}

void DirectXTransitionManager::TransitionLayout(ID3D12GraphicsCommandList7* cmd, ID3D12Resource* resource, D3D12_RESOURCE_STATES layout)
{
	D3D12_RESOURCE_STATES oldLayout = GetLayout(resource);
	if ((oldLayout & layout) == layout) { return; }
	
	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrier.Transition.pResource = resource;
	barrier.Transition.StateBefore = oldLayout;
	barrier.Transition.StateAfter = layout;
	cmd->ResourceBarrier(1, &barrier);
	SetLayout(resource, layout);
}

void DirectXTransitionManager::SetLayout(ID3D12Resource* resource, D3D12_RESOURCE_STATES layout)
{
	mResourceLayoutCache[resource] = layout;
}

D3D12_RESOURCE_STATES DirectXTransitionManager::GetLayout(ID3D12Resource* resource)
{
	D3D12_RESOURCE_STATES layout = D3D12_RESOURCE_STATE_COMMON;
	auto it = mResourceLayoutCache.find(resource);
	if (it != mResourceLayoutCache.end())
	{
		layout = it->second;
	}
	return layout;
}

void DirectXTransitionManager::Clear()
{
	mResourceLayoutCache.clear();
}

#endif
