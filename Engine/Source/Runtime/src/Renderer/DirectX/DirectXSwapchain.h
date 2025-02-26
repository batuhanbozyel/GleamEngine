#pragma once
#ifdef USE_DIRECTX_RENDERER
#include "Renderer/Swapchain.h"

#include <d3d12.h>
#include <dxgi1_6.h>

#ifdef GDEBUG
#include <d3d12sdklayers.h>
#include <dxgidebug.h>
#endif

namespace Gleam {

class DirectXDevice;

class DirectXSwapchain final : public Swapchain
{
	friend class DirectXDevice;

public:

	DirectXSwapchain();

	~DirectXSwapchain();

	void Configure(DirectXDevice* device, const RendererConfig& config);

	virtual const Texture& AcquireNextDrawable() override;

	virtual void Resize(GraphicsDevice* device, const Size& size) override;

	virtual void Present(const CommandBuffer* cmd) override;

	virtual TextureFormat GetFormat() const override;

	virtual const Size& GetSize() const override;

private:

	Texture CreateSwapchainBuffer(GraphicsDevice* device, uint32_t buffer);

	void ReleaseSwapchainBuffer(GraphicsDevice* device, Texture& texture);

#ifdef GDEBUG
	IDXGIDebug1* mDXGIDebug = nullptr;
#endif

	DXGI_SWAP_CHAIN_DESC1 mDesc = {};

	IDXGIAdapter4* mAdapter = nullptr;

	IDXGIFactory7* mFactory = nullptr;

	IDXGISwapChain4* mHandle = nullptr;

	DirectXDevice* mDevice = nullptr;

	struct FrameContext
	{
		ID3D12Fence* fence;
		uint64_t waitFenceValue = 0;
		uint64_t fenceValue = 0;
	};
	TArray<FrameContext> mContext;

};

} // Gleam
#endif