#include "gpch.h"

#ifdef USE_DIRECTX_RENDERER
#include "DirectXSwapchain.h"
#include "DirectXDevice.h"
#include "DirectXUtils.h"

#include "Core/Engine.h"
#include "Core/Globals.h"
#include "Core/WindowSystem.h"

using namespace Gleam;

DirectXSwapchain::DirectXSwapchain()
{
	UINT dxgiFactoryFlags = 0;
#ifdef GDEBUG
	if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&mDXGIDebug))))
	{
		dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
		mDXGIDebug->EnableLeakTrackingForThread();
	}
#endif
	DX_CHECK(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&mFactory)));
	DX_CHECK(mFactory->EnumAdapterByGpuPreference(0, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&mAdapter)));
}

DirectXSwapchain::~DirectXSwapchain()
{
	auto& ctx = mContext[mCurrentFrameIndex];
	WaitForID3D12Fence(ctx.fence, ctx.waitFenceValue);

	for (auto& texture : mTextures)
	{
		static_cast<ID3D12Resource*>(texture.GetHandle())->Release();
	}
	mTextures.clear();

	for (auto& ctx : mContext)
	{
		ctx.fence->Release();
	}
	mContext.clear();

	mFactory->Release();
	mAdapter->Release();
	mHandle->Release();

#ifdef GDEBUG
	if (mDXGIDebug)
	{
		OutputDebugStringW(L"DXGI Reports living device objects:\n");
		mDXGIDebug->ReportLiveObjects(DXGI_DEBUG_ALL,
			DXGI_DEBUG_RLO_FLAGS(DXGI_DEBUG_RLO_DETAIL | DXGI_DEBUG_RLO_IGNORE_INTERNAL)
		);

		mDXGIDebug->Release();
	}
#endif
}

void DirectXSwapchain::Configure(DirectXDevice* device, const RendererConfig& config)
{
	mMaxFramesInFlight = config.tripleBufferingEnabled ? 3 : 2;
	mCurrentFrameIndex = 0;
	mDevice = device;

	int width, height;
	auto windowSystem = Globals::Engine->GetSubsystem<WindowSystem>();
	SDL_GetWindowSizeInPixels(windowSystem->GetSDLWindow(), &width, &height);

	mDesc =
	{
		.Width = (UINT)width,
		.Height = (UINT)height,
		.Format = DXGI_FORMAT_B8G8R8A8_UNORM,
		.SampleDesc = {.Count = 1, .Quality = 0 },
		.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
		.BufferCount = mMaxFramesInFlight,
		.Scaling = DXGI_SCALING_STRETCH,
		.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD,
		.AlphaMode = DXGI_ALPHA_MODE_IGNORE,
		.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH
	};

	if (config.vsync == false)
	{
		mDesc.Flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
	}

	if (mHandle != nullptr)
	{
		for (auto& ctx : mContext)
		{
			ctx.fence->Release();
		}

		mHandle->Release();
		mHandle = nullptr;
	}

	mContext.resize(mMaxFramesInFlight);
	for (auto& ctx : mContext)
	{
		DX_CHECK(static_cast<ID3D12Device10*>(mDevice->GetHandle())->CreateFence(
			0,
			D3D12_FENCE_FLAG_NONE,
			IID_PPV_ARGS(&ctx.fence)
		));
	}
	Resize(mDevice, Size((float)width, (float)height));
}

void DirectXSwapchain::Resize(GraphicsDevice* device, const Size& size)
{
	auto& ctx = mContext[mCurrentFrameIndex];
	WaitForID3D12Fence(ctx.fence, ctx.waitFenceValue);

	mDesc.Width = (UINT)size.width;
	mDesc.Height = (UINT)size.height;

	if (mHandle != nullptr)
	{
		// Destroy old context
		for (auto& texture : mTextures)
		{
			ReleaseSwapchainBuffer(device, texture);
		}
		mHandle->ResizeBuffers(mMaxFramesInFlight, mDesc.Width, mDesc.Height, mDesc.Format, 0);
	}
	else
	{
		SDL_Window* window = Globals::Engine->GetSubsystem<WindowSystem>()->GetSDLWindow();
		HWND hwnd = (HWND)SDL_GetPointerProperty(SDL_GetWindowProperties(window), SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);

		IDXGISwapChain1* swapchain1 = nullptr;
		DX_CHECK(mFactory->CreateSwapChainForHwnd(static_cast<DirectXDevice*>(device)->GetDirectQueue(), hwnd, &mDesc, nullptr, nullptr, &swapchain1));
		DX_CHECK(swapchain1->QueryInterface(IID_PPV_ARGS(&mHandle)));
		swapchain1->Release();
	}

	mTextures.resize(mMaxFramesInFlight);
	for (uint32_t i = 0; i < mMaxFramesInFlight; i++)
	{
		mTextures[i] = CreateSwapchainBuffer(device, i);
	}
	mCurrentFrameIndex = mHandle->GetCurrentBackBufferIndex();
}

const Texture& DirectXSwapchain::AcquireNextDrawable()
{
	mCurrentFrameIndex = mHandle->GetCurrentBackBufferIndex();

	auto& ctx = mContext[mCurrentFrameIndex];
	WaitForID3D12Fence(ctx.fence, ctx.waitFenceValue);

	return mTextures[mCurrentFrameIndex];
}

void DirectXSwapchain::Present(const CommandBuffer* cmd)
{
	auto& ctx = mContext[mCurrentFrameIndex];
	D3D12_TEXTURE_BARRIER textureBarrier = {
		.SyncBefore = D3D12_BARRIER_SYNC_RENDER_TARGET,
		.SyncAfter = D3D12_BARRIER_SYNC_NONE,
		.AccessBefore = D3D12_BARRIER_ACCESS_RENDER_TARGET,
		.AccessAfter = D3D12_BARRIER_ACCESS_NO_ACCESS,
		.LayoutBefore = D3D12_BARRIER_LAYOUT_RENDER_TARGET,
		.LayoutAfter = D3D12_BARRIER_LAYOUT_PRESENT,
		.pResource = static_cast<ID3D12Resource*>(mTextures[mCurrentFrameIndex].GetHandle()),
		.Subresources = {
			.IndexOrFirstMipLevel = 0xffffffff,
			.NumMipLevels = 0,
			.FirstArraySlice = 0,
			.NumArraySlices = 0,
			.FirstPlane = 0,
			.NumPlanes = 0
		},
		.Flags = D3D12_TEXTURE_BARRIER_FLAG_NONE
	};

	D3D12_BARRIER_GROUP barrier = {
		.Type = D3D12_BARRIER_TYPE_TEXTURE,
		.NumBarriers = 1,
		.pTextureBarriers = &textureBarrier
	};
	static_cast<ID3D12GraphicsCommandList7*>(cmd->GetHandle())->Barrier(1, &barrier);
	
	cmd->End();
	cmd->Commit();

	if (mDesc.Flags & DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING)
	{
		mHandle->Present(0, DXGI_PRESENT_ALLOW_TEARING);
	}
	else
	{
		mHandle->Present(1, 0);
	}

	ctx.waitFenceValue = ctx.fenceValue++;
	DX_CHECK(mDevice->GetDirectQueue()->Signal(ctx.fence, ctx.fenceValue));

	mCurrentFrameIndex = mHandle->GetCurrentBackBufferIndex();
}

Texture DirectXSwapchain::CreateSwapchainBuffer(GraphicsDevice* device, uint32_t buffer)
{
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc =
	{
		.Format = mDesc.Format,
		.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D
	};

	ID3D12Resource* texture = nullptr;
	RenderTargetView rtv = static_cast<DirectXDevice*>(device)->GetRtvHeap().Allocate();

	DX_CHECK(mHandle->GetBuffer(buffer, IID_PPV_ARGS(&texture)));
	static_cast<ID3D12Device10*>(device->GetHandle())->CreateRenderTargetView(texture, &rtvDesc, rtv);

	TStringStream ss;
	ss << "Swapchain::Drawable_" << buffer;

	TWString resourceName = ss.str();
	texture->SetName(resourceName.c_str());

	TextureDescriptor swapchainDesc;
	swapchainDesc.name = resourceName;
	swapchainDesc.dimension = TextureDimension::Texture2D;
	swapchainDesc.size = Size((float)mDesc.Width, (float)mDesc.Height);
	swapchainDesc.usage = TextureUsage_Attachment;
	swapchainDesc.format = DXGI_FORMATtoTextureFormat(mDesc.Format);
	return Texture(swapchainDesc, texture, rtv);
}

void DirectXSwapchain::ReleaseSwapchainBuffer(GraphicsDevice* device, Texture& texture)
{
	static_cast<ID3D12Resource*>(texture.GetHandle())->Release();
	static_cast<DirectXDevice*>(device)->GetRtvHeap().Release(texture.GetRenderTargetView());
	texture = Texture();
}

#endif