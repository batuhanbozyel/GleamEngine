#pragma once
#ifdef USE_DIRECTX_RENDERER
#include "DirectXDevice.h"

namespace Gleam {

class DirectXTransitionManager
{
public:

	static void Init(DirectXDevice* device);
	
	static void TransitionLayout(ID3D12GraphicsCommandList7* cmd, ID3D12Resource* resource, D3D12_RESOURCE_STATES layout);

	static void SetLayout(ID3D12Resource* resource, D3D12_RESOURCE_STATES layout);

	static D3D12_RESOURCE_STATES GetLayout(ID3D12Resource* resource);

	static void RemoveResource(ID3D12Resource* resource);

	static void Clear();

private:

	static inline DirectXDevice* mDevice = nullptr;

	static inline HashMap<ID3D12Resource*, D3D12_RESOURCE_STATES> mResourceLayoutCache;

};

} // namespace Gleam
#endif
