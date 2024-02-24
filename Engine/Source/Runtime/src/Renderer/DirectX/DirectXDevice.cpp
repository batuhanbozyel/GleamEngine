#include "gpch.h"

#ifdef USE_DIRECTX_RENDERER
#include "DirectXDevice.h"
#include "DirectXUtils.h"
#include "DirectXShaderReflect.h"
#include "DirectXTransitionManager.h"
#include "DirectXPipelineStateManager.h"

#include "Core/Application.h"
#include "Core/WindowSystem.h"

using namespace Gleam;

Scope<GraphicsDevice> GraphicsDevice::Create()
{
	return CreateScope<DirectXDevice>();
}

DirectXDevice::DirectXDevice()
{
	UINT dxgiFactoryFlags = 0;
#ifdef GDEBUG
	ID3D12Debug6* debugController = nullptr;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
	{
		dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;

		debugController->EnableDebugLayer();
		debugController->SetEnableGPUBasedValidation(true);
		debugController->Release();
	}
#endif

	DX_CHECK(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&mFactory)));

	DX_CHECK(mFactory->EnumAdapterByGpuPreference(0, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&mAdapter)));

	DX_CHECK(D3D12CreateDevice(mAdapter, D3D_FEATURE_LEVEL_12_0, __uuidof(ID3D12Device10), &mHandle));

	D3D12_COMMAND_QUEUE_DESC commandQueueDesc = {};
	commandQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;

	// Direct queue
	commandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	DX_CHECK(static_cast<ID3D12Device10*>(mHandle)->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&mDirectQueue)));

	// Compute queue
	commandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
	DX_CHECK(static_cast<ID3D12Device10*>(mHandle)->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&mComputeQueue)));

	// Copy queue
	commandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_COPY;
	DX_CHECK(static_cast<ID3D12Device10*>(mHandle)->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&mCopyQueue)));

	DirectXPipelineStateManager::Init(this);

	GLEAM_CORE_INFO("DirectX: Graphics device created.");
}

DirectXDevice::~DirectXDevice()
{
	DirectXPipelineStateManager::Destroy();
	mSwapchain->Release();
	mFactory->Release();
	mAdapter->Release();
	static_cast<ID3D12Device10*>(mHandle)->Release();

	GLEAM_CORE_INFO("DirectX: Graphics device destroyed.");
}

void DirectXDevice::Configure(const RendererConfig& config)
{
	auto windowSystem = GameInstance->GetSubsystem<WindowSystem>();

	int width, height;
	SDL_GetWindowSizeInPixels(windowSystem->GetSDLWindow(), &width, &height);
	mSize.width = static_cast<float>(width);
	mSize.height = static_cast<float>(height);
	mMaxFramesInFlight = config.tripleBufferingEnabled ? 3 : 2;

	if (mSwapchain != nullptr)
	{
		mSwapchain->ResizeBuffers(mMaxFramesInFlight, (UINT)mSize.width, (UINT)mSize.height, TextureFormatToDXGI_FORMAT(mFormat), 0);
	}
	else
	{
		DXGI_FORMAT format = DXGI_FORMAT_B8G8R8A8_UNORM;
		mFormat = DXGI_FORMATtoTextureFormat(format);

		DXGI_SWAP_CHAIN_DESC1 swapchainDesc =
		{
			.Width = (UINT)mSize.width,
			.Height = (UINT)mSize.height,
			.Format = DXGI_FORMAT_B8G8R8A8_UNORM,
			.SampleDesc = {.Count = 1, .Quality = 0 },
			.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
			.BufferCount = mMaxFramesInFlight,
			.Scaling = DXGI_SCALING_STRETCH,
			.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD,
			.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED,
			.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING
		};

		SDL_Window* window = GameInstance->GetSubsystem<WindowSystem>()->GetSDLWindow();
		HWND hwnd = (HWND)SDL_GetProperty(SDL_GetWindowProperties(window), SDL_PROPERTY_WINDOW_WIN32_HWND_POINTER, NULL);

		IDXGISwapChain1* swapchain1 = nullptr;
		DX_CHECK(mFactory->CreateSwapChainForHwnd(mDirectQueue, hwnd, &swapchainDesc, nullptr, nullptr, &swapchain1));
		DX_CHECK(swapchain1->QueryInterface(IID_PPV_ARGS(&mSwapchain)));
		swapchain1->Release();
	}

	mCurrentFrameIndex = mSwapchain->GetCurrentBackBufferIndex();

}

void DirectXDevice::DestroyFrameObjects(uint32_t frameIndex)
{
	/*
		TODO:
	*/
}

ID3D12CommandQueue* DirectXDevice::GetDirectQueue() const
{
	return mDirectQueue;
}

ID3D12CommandQueue* DirectXDevice::GetComputeQueue() const
{
	return mComputeQueue;
}

ID3D12CommandQueue* DirectXDevice::GetCopyQueue() const
{
	return mCopyQueue;
}

#endif
