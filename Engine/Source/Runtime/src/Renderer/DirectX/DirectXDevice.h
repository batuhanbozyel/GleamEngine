#pragma once
#ifdef USE_DIRECTX_RENDERER
#include "Renderer/GraphicsDevice.h"

#include <d3d12.h>
#include <dxgi1_6.h>
#include <D3D12MemAlloc.h>

namespace Gleam {

class DirectXDevice final : public GraphicsDevice
{
public:

	DirectXDevice();

    ~DirectXDevice();

	ID3D12CommandQueue* GetDirectQueue() const;

	ID3D12CommandQueue* GetComputeQueue() const;

	ID3D12CommandQueue* GetCopyQueue() const;

private:

	virtual void DestroyFrameObjects(uint32_t frameIndex) override;

	virtual void Configure(const RendererConfig& config) override;

	IDXGISwapChain4* mSwapchain = nullptr;

	IDXGIAdapter4* mAdapter = nullptr;

	IDXGIFactory7* mFactory = nullptr;

	ID3D12CommandQueue* mDirectQueue = nullptr;

	ID3D12CommandQueue* mComputeQueue = nullptr;

	ID3D12CommandQueue* mCopyQueue = nullptr;

};

} // namespace Gleam
#endif
