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

Heap GraphicsDevice::AllocateHeap(const HeapDescriptor& descriptor) const
{
	Heap heap(descriptor);
	heap.mDevice = this;

	return heap;
}

Texture GraphicsDevice::AllocateTexture(const TextureDescriptor& descriptor) const
{
	Texture texture(descriptor);
	return texture;
}

Shader GraphicsDevice::GenerateShader(const TString& entryPoint, ShaderStage stage) const
{
	Shader shader(entryPoint, stage);

	auto& shaderFile = GameInstance->GetDefaultAssetPath().append("Shaders/" + entryPoint + ".dxil");
	auto bytecodeLength = IOUtils::QueryFileBufferSize(shaderFile);
	auto bytecode = new char[bytecodeLength];
	IOUtils::ReadBinaryFile(shaderFile, static_cast<char*>(bytecode), bytecodeLength);

	D3D12_SHADER_BYTECODE* shaderBytecode = new D3D12_SHADER_BYTECODE;
	shaderBytecode->pShaderBytecode = bytecode;
	shaderBytecode->BytecodeLength = bytecodeLength;

	shader.mReflection = CreateScope<Shader::Reflection>(this, shaderBytecode);
	shader.mHandle = shaderBytecode;

	return shader;
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
	DX_CHECK(static_cast<ID3D12Device10*>(mHandle)->CreateCommandAllocator(commandQueueDesc.Type, IID_PPV_ARGS(&mDirectCommandAllocator)));

	// Compute queue
	commandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
	DX_CHECK(static_cast<ID3D12Device10*>(mHandle)->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&mComputeQueue)));
	DX_CHECK(static_cast<ID3D12Device10*>(mHandle)->CreateCommandAllocator(commandQueueDesc.Type, IID_PPV_ARGS(&mComputeCommandAllocator)));

	// Copy queue
	commandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_COPY;
	DX_CHECK(static_cast<ID3D12Device10*>(mHandle)->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&mCopyQueue)));
	DX_CHECK(static_cast<ID3D12Device10*>(mHandle)->CreateCommandAllocator(commandQueueDesc.Type, IID_PPV_ARGS(&mCopyCommandAllocator)));

	DX_CHECK(DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&mDxcUtils)));
	DirectXPipelineStateManager::Init(this);

	GLEAM_CORE_INFO("DirectX: Graphics device created.");
}

DirectXDevice::~DirectXDevice()
{
	for (auto& shader : mShaderCache)
	{
		auto bytecode = static_cast<D3D12_SHADER_BYTECODE*>(shader.GetHandle());
		delete bytecode->pShaderBytecode;
		delete bytecode;
	}
	mShaderCache.clear();

	DirectXPipelineStateManager::Destroy();

	mDirectQueue->Release();
	mDirectCommandAllocator->Release();

	mComputeQueue->Release();
	mComputeCommandAllocator->Release();

	mCopyQueue->Release();
	mCopyCommandAllocator->Release();

	mDxcUtils->Release();
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
		// Destroy frame objects
		for (uint32_t frameIndex = 0; frameIndex < mFrameObjects.size(); frameIndex++)
		{
			DestroyFrameObjects(frameIndex);
		}
		mFrameObjects.clear();

		// Destroy RenderTargets
		for (auto renderTarget : mRenderTargets)
		{
			renderTarget->Release();
		}
		mRenderTargets.clear();

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
			.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH
		};
		swapchainDesc.Flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

		SDL_Window* window = GameInstance->GetSubsystem<WindowSystem>()->GetSDLWindow();
		HWND hwnd = (HWND)SDL_GetProperty(SDL_GetWindowProperties(window), SDL_PROPERTY_WINDOW_WIN32_HWND_POINTER, NULL);

		IDXGISwapChain1* swapchain1 = nullptr;
		DX_CHECK(mFactory->CreateSwapChainForHwnd(mDirectQueue, hwnd, &swapchainDesc, nullptr, nullptr, &swapchain1));
		DX_CHECK(swapchain1->QueryInterface(IID_PPV_ARGS(&mSwapchain)));
		swapchain1->Release();
	}

	mCurrentFrameIndex = mSwapchain->GetCurrentBackBufferIndex();
	mFrameObjects.resize(mMaxFramesInFlight);

	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc{};
	rtvHeapDesc.NumDescriptors = mMaxFramesInFlight;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	DX_CHECK(static_cast<ID3D12Device10*>(mHandle)->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&mRTVHeap)));

	UINT rtvDescriptorSize = static_cast<ID3D12Device10*>(mHandle)->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle(mRTVHeap->GetCPUDescriptorHandleForHeapStart());

	mRenderTargets.resize(mMaxFramesInFlight);
	for (uint32_t i = 0; i < mMaxFramesInFlight; i++)
	{
		DX_CHECK(mSwapchain->GetBuffer(i, IID_PPV_ARGS(&mRenderTargets[i])));
		static_cast<ID3D12Device10*>(mHandle)->CreateRenderTargetView(mRenderTargets[i], nullptr, rtvHandle);
		rtvHandle.ptr += rtvDescriptorSize;
	}
}

DirectXDrawable DirectXDevice::AcquireNextDrawable()
{
	mCurrentFrameIndex = mSwapchain->GetCurrentBackBufferIndex();
	UINT rtvDescriptorSize = static_cast<ID3D12Device10*>(mHandle)->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	DirectXDrawable drawable{};
	drawable.descriptor = mRTVHeap->GetCPUDescriptorHandleForHeapStart();
	drawable.descriptor.ptr = drawable.descriptor.ptr + SIZE_T(mCurrentFrameIndex * rtvDescriptorSize);
	drawable.renderTarget = mRenderTargets[mCurrentFrameIndex];
	return drawable;
}

void DirectXDevice::Present(ID3D12GraphicsCommandList7* commandList)
{
	D3D12_RESOURCE_BARRIER renderTargetBarrier{};
	renderTargetBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	renderTargetBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	renderTargetBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

	renderTargetBarrier.Transition.pResource = mRenderTargets[mCurrentFrameIndex];
	renderTargetBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	renderTargetBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	commandList->ResourceBarrier(1, &renderTargetBarrier);

	DXGI_SWAP_CHAIN_DESC1 swapchainDesc{};
	mSwapchain->GetDesc1(&swapchainDesc);
	if (swapchainDesc.Flags & DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING)
	{
		mSwapchain->Present(0, DXGI_PRESENT_ALLOW_TEARING);
	}
	else
	{
		mSwapchain->Present(1, 0);
	}
}

void DirectXDevice::DestroyFrameObjects(uint32_t frameIndex)
{
	auto& pool = mFrameObjects[frameIndex];
	for (const auto& commandList : pool.commandLists)
	{
		commandList.handle->Reset(commandList.allocator, nullptr);
	}
}

ID3D12GraphicsCommandList7* DirectXDevice::AllocateCommandList(D3D12_COMMAND_LIST_TYPE type)
{
	DirectXCommandList commandList{};
	switch (type)
	{
		case D3D12_COMMAND_LIST_TYPE_DIRECT:
		{
			commandList.allocator = mDirectCommandAllocator;
			break;
		}
		case D3D12_COMMAND_LIST_TYPE_COMPUTE:
		{
			commandList.allocator = mComputeCommandAllocator;
			break;
		}
		case D3D12_COMMAND_LIST_TYPE_COPY:
		{
			commandList.allocator = mCopyCommandAllocator;
			break;
		}
		default:
		{
			GLEAM_ASSERT(false, "DirectX: Command list type is not supported!");
			return nullptr;
		}
	}
	DX_CHECK(static_cast<ID3D12Device10*>(mHandle)->CreateCommandList(0, type, commandList.allocator, nullptr, IID_PPV_ARGS(&commandList.handle)));
	mFrameObjects[mCurrentFrameIndex].commandLists.push_back(commandList);
	return commandList.handle;
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

IDxcUtils* DirectXDevice::GetDxcUtils() const
{
	return mDxcUtils;
}

#endif
