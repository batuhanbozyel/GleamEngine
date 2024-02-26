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

MemoryRequirements GraphicsDevice::QueryMemoryRequirements(const HeapDescriptor& descriptor) const
{
	D3D12_HEAP_PROPERTIES heapProperties = {
		.Type = D3D12_HEAP_TYPE_DEFAULT,
		.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
		.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
		.CreationNodeMask = 1,
		.VisibleNodeMask = 1
	};

	D3D12_RESOURCE_DESC resourceDesc = {
		.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
		.Alignment = 0,
		.Width = descriptor.size,
		.Height = 1,
		.DepthOrArraySize = 1,
		.MipLevels = 1,
		.Format = DXGI_FORMAT_UNKNOWN,
		.SampleDesc = { .Count = 1, .Quality = 0 },
		.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
		.Flags = D3D12_RESOURCE_FLAG_NONE
	};

	if (descriptor.memoryType == MemoryType::GPU)
	{
		resourceDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	}

	D3D12_RESOURCE_ALLOCATION_INFO allocationInfo = static_cast<ID3D12Device10*>(mHandle)->GetResourceAllocationInfo(0, 1, &resourceDesc);
	return MemoryRequirements
	{
		.size = allocationInfo.SizeInBytes,
		.alignment = allocationInfo.Alignment
	};
}

Heap GraphicsDevice::AllocateHeap(const HeapDescriptor& descriptor) const
{
	Heap heap(descriptor);
	heap.mDevice = this;

	auto memoryRequirements = QueryMemoryRequirements(descriptor);
	heap.mDescriptor.size = memoryRequirements.size;
	heap.mAlignment = memoryRequirements.alignment;

	D3D12_HEAP_DESC desc{};
	desc.Alignment = heap.mAlignment;
	desc.SizeInBytes = heap.mDescriptor.size;
	desc.Flags = D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS;
	desc.Properties.Type = descriptor.memoryType == MemoryType::CPU ? D3D12_HEAP_TYPE_UPLOAD : D3D12_HEAP_TYPE_DEFAULT;
	static_cast<ID3D12Device10*>(mHandle)->CreateHeap(&desc, __uuidof(ID3D12Heap*), &heap.mHandle);
	return heap;
}

Texture GraphicsDevice::AllocateTexture(const TextureDescriptor& descriptor) const
{
	Texture texture(descriptor);

	D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE;
	D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_COPY_SOURCE | D3D12_RESOURCE_STATE_COPY_DEST;

	if (descriptor.usage & TextureUsage_Attachment)
	{
		if (Utils::IsColorFormat(descriptor.format))
		{
			flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
			initialState |= D3D12_RESOURCE_STATE_RENDER_TARGET;
		}
		else
		{
			flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
			initialState |= D3D12_RESOURCE_STATE_DEPTH_WRITE;
		}
	}

	if (descriptor.usage & TextureUsage_Storage)
	{
		flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		initialState |= D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	}

	if (descriptor.usage & TextureUsage_Sampled)
	{
		if (Utils::IsColorFormat(descriptor.format))
		{
			initialState |= D3D12_RESOURCE_STATE_GENERIC_READ;
		}
		else
		{
			initialState |= D3D12_RESOURCE_STATE_DEPTH_READ;
		}
	}

	D3D12_RESOURCE_DESC resourceDesc = {
		.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
		.Alignment = 0,
		.Width = (UINT64)descriptor.size.width,
		.Height = (UINT64)descriptor.size.height,
		.DepthOrArraySize = 1,
		.MipLevels = texture.mMipMapLevels,
		.Format = DXGI_FORMAT_UNKNOWN,
		.SampleDesc = {.Count = descriptor.sampleCount, .Quality = 0 },
		.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
		.Flags = flags
	};

	D3D12_HEAP_PROPERTIES heapProperties = {
		.Type = D3D12_HEAP_TYPE_DEFAULT,
		.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
		.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
		.CreationNodeMask = 1,
		.VisibleNodeMask = 1
	};

	static_cast<ID3D12Device10*>(mHandle)->CreateCommittedResource(
		&heapProperties,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		initialState,
		nullptr,
		__uuidof(ID3D12Resource*),
		&texture.mHandle
	);
	DirectXTransitionManager::SetLayout(static_cast<ID3D12Resource*>(texture.mHandle), initialState);

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

void GraphicsDevice::Dispose(Heap& heap) const
{
	static_cast<ID3D12Heap*>(heap.mHandle)->Release();
	heap.mHandle = nullptr;
}

void GraphicsDevice::Dispose(Buffer& buffer) const
{
	static_cast<ID3D12Resource*>(buffer.mHandle)->Release();
	buffer.mHandle = nullptr;
}

void GraphicsDevice::Dispose(Texture& texture) const
{
	static_cast<ID3D12Resource*>(texture.mHandle)->Release();
	texture.mHandle = nullptr;
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

	mDirectQueue = CreateCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);
	mComputeQueue = CreateCommandQueue(D3D12_COMMAND_LIST_TYPE_COMPUTE);
	mCopyQueue = CreateCommandQueue(D3D12_COMMAND_LIST_TYPE_COPY);

	mRtvHeap = CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	mDsvHeap = CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	mCbvSrvUavHeap = CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	
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

	for (auto& ctx : mFrameContext)
	{
		ctx.commandPool.Release();
		ctx.renderTarget->Release();
		ctx.fence->Release();
	}
	mFrameContext.clear();

	DirectXPipelineStateManager::Destroy();

	mRtvHeap.handle->Release();
	mDsvHeap.handle->Release();
	mCbvSrvUavHeap.handle->Release();

	mDirectQueue->Release();
	mComputeQueue->Release();
	mCopyQueue->Release();

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
		// Destroy old context
		for (auto& ctx : mFrameContext)
		{
			ctx.commandPool.Release();
			ctx.renderTarget->Release();
			ctx.fence->Release();
		}
		mFrameContext.clear();

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

	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle(mRtvHeap.handle->GetCPUDescriptorHandleForHeapStart());

	mFrameContext.resize(mMaxFramesInFlight);
	for (uint32_t i = 0; i < mMaxFramesInFlight; i++)
	{
		auto& ctx = mFrameContext[i];

		// create swapchain RTV
		DX_CHECK(mSwapchain->GetBuffer(i, IID_PPV_ARGS(&ctx.renderTarget)));
		static_cast<ID3D12Device10*>(mHandle)->CreateRenderTargetView(ctx.renderTarget, nullptr, rtvHandle);
		rtvHandle.ptr += mRtvHeap.size;

		// create fence
		DX_CHECK(static_cast<ID3D12Device10*>(mHandle)->CreateFence(
			ctx.frameCount,
			D3D12_FENCE_FLAG_NONE,
			IID_PPV_ARGS(&ctx.fence)
		));
	}
	mCurrentFrameIndex = mSwapchain->GetCurrentBackBufferIndex();
}

DirectXDrawable DirectXDevice::AcquireNextDrawable()
{
	auto& ctx = mFrameContext[mCurrentFrameIndex];
	WaitForID3D12Fence(ctx.fence, ctx.frameCount);

	DirectXDrawable drawable{};
	drawable.descriptor = mRtvHeap.handle->GetCPUDescriptorHandleForHeapStart();
	drawable.descriptor.ptr = drawable.descriptor.ptr + SIZE_T(mCurrentFrameIndex * mRtvHeap.size);
	drawable.renderTarget = ctx.renderTarget;
	return drawable;
}

void DirectXDevice::Present(const CommandBuffer* cmd)
{
	auto& ctx = mFrameContext[mCurrentFrameIndex];

	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	barrier.Transition.pResource = ctx.renderTarget;

	static_cast<ID3D12GraphicsCommandList7*>(cmd->GetHandle())->ResourceBarrier(1, &barrier);
	cmd->Commit();

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

	mDirectQueue->Signal(ctx.fence, ++ctx.frameCount);
	mCurrentFrameIndex = mSwapchain->GetCurrentBackBufferIndex();
}

DirectXCommandList DirectXDevice::AllocateCommandList(D3D12_COMMAND_LIST_TYPE type)
{
	auto& pool = mFrameContext[mCurrentFrameIndex].commandPool;

	DirectXCommandList commandList{};
	if (pool.freeCommandLists.empty())
	{
		DX_CHECK(static_cast<ID3D12Device10*>(mHandle)->CreateCommandAllocator(type, IID_PPV_ARGS(&commandList.allocator)));
		DX_CHECK(static_cast<ID3D12Device10*>(mHandle)->CreateCommandList1(0, type, D3D12_COMMAND_LIST_FLAG_NONE, IID_PPV_ARGS(&commandList.handle)));
		pool.usedCommandLists.push_back(commandList);
	}
	else
	{
		commandList = pool.freeCommandLists.front();
		pool.freeCommandLists.pop_front();
	}

	commandList.handle->Reset(commandList.allocator, nullptr);
	commandList.allocator->Reset();

	return commandList;
}

ID3D12CommandQueue* DirectXDevice::CreateCommandQueue(D3D12_COMMAND_LIST_TYPE type) const
{
	D3D12_COMMAND_QUEUE_DESC desc{};
	desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	desc.Type = type;

	ID3D12CommandQueue* queue = nullptr;
	DX_CHECK(static_cast<ID3D12Device10*>(mHandle)->CreateCommandQueue(&desc, IID_PPV_ARGS(&queue)));
	return queue;
}

DirectXDescriptorHeap DirectXDevice::CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type) const
{
	D3D12_DESCRIPTOR_HEAP_DESC desc{};
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	desc.NumDescriptors = D3D12_MAX_SHADER_VISIBLE_DESCRIPTOR_HEAP_SIZE_TIER_2;
	desc.Type = type;

	DirectXDescriptorHeap heap{};
	DX_CHECK(static_cast<ID3D12Device10*>(mHandle)->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&heap.handle)));
	heap.size = static_cast<ID3D12Device10*>(mHandle)->GetDescriptorHandleIncrementSize(type);
	return heap;
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

void DirectXDevice::CommandPool::Reset()
{
	freeCommandLists.insert(freeCommandLists.end(), usedCommandLists.begin(), usedCommandLists.end());
	usedCommandLists.clear();
}

void DirectXDevice::CommandPool::Release()
{
	for (auto& cmd : usedCommandLists)
	{
		cmd.handle->Release();
		cmd.allocator->Release();
	}

	for (auto& cmd : freeCommandLists)
	{
		cmd.handle->Release();
		cmd.allocator->Release();
	}
}

#endif
