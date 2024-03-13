#include "gpch.h"

#ifdef USE_DIRECTX_RENDERER
#include "DirectXDevice.h"
#include "DirectXUtils.h"
#include "DirectXTransitionManager.h"
#include "DirectXPipelineStateManager.h"

#include "Core/Application.h"
#include "Core/WindowSystem.h"

using namespace Gleam;

static void DirectXDebugCallback(D3D12_MESSAGE_CATEGORY Category,
								 D3D12_MESSAGE_SEVERITY Severity,
								 D3D12_MESSAGE_ID ID,
								 LPCSTR pDescription,
								 void* pContext)
{
	if (Severity & D3D12_MESSAGE_SEVERITY_ERROR)
	{
		GLEAM_CORE_ERROR("DirectX: {0}", pDescription);
		GLEAM_ASSERT(false);
	}
	else if (Severity & D3D12_MESSAGE_SEVERITY_WARNING)
	{
		GLEAM_CORE_WARN("DirectX: {0}", pDescription);
	}
	else if (Severity & D3D12_MESSAGE_SEVERITY_INFO)
	{
		GLEAM_CORE_INFO("DirectX: {0}", pDescription);
	}
	else
	{
		GLEAM_CORE_TRACE("DirectX: {0}", pDescription);
	}
}

Scope<GraphicsDevice> GraphicsDevice::Create()
{
	return CreateScope<DirectXDevice>();
}

MemoryRequirements GraphicsDevice::QueryMemoryRequirements(const HeapDescriptor& descriptor) const
{
	D3D12_HEAP_PROPERTIES heapProperties = {
		.Type = descriptor.memoryType == MemoryType::CPU ? D3D12_HEAP_TYPE_UPLOAD : D3D12_HEAP_TYPE_DEFAULT,
		.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
		.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
		.CreationNodeMask = 0,
		.VisibleNodeMask = 0
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

Heap GraphicsDevice::AllocateHeap(const HeapDescriptor& descriptor)
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
	DX_CHECK(static_cast<ID3D12Device10*>(mHandle)->CreateHeap(&desc, __uuidof(ID3D12Heap*), &heap.mHandle));
	return heap;
}

Texture GraphicsDevice::AllocateTexture(const TextureDescriptor& descriptor)
{
	Texture texture(descriptor);

	D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE;
	if (descriptor.usage & TextureUsage_Attachment)
	{
		if (Utils::IsColorFormat(descriptor.format))
		{
			flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
		}
		else
		{
			flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
		}
	}

	if (descriptor.usage & TextureUsage_Storage)
	{
		flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	}

	D3D12_RESOURCE_DESC resourceDesc = {
		.Dimension = TextureDimensionToD3D12_RESOURCE_DIMENSION(descriptor.dimension),
		.Alignment = 0,
		.Width = (UINT64)descriptor.size.width,
		.Height = (UINT64)descriptor.size.height,
		.DepthOrArraySize = (UINT16)(descriptor.dimension == TextureDimension::TextureCube ? 6 : 1),
		.MipLevels = (UINT16)texture.mMipMapLevels,
		.Format = TextureFormatToDXGI_FORMAT(descriptor.format),
		.SampleDesc = {.Count = 1, .Quality = 0 },
		.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
		.Flags = flags
	};

	D3D12_HEAP_PROPERTIES heapProperties = {
		.Type = D3D12_HEAP_TYPE_DEFAULT,
		.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
		.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
		.CreationNodeMask = 0,
		.VisibleNodeMask = 0
	};

	// TODO: Create MSAA texture
	DX_CHECK(static_cast<ID3D12Device10*>(mHandle)->CreateCommittedResource(
		&heapProperties,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		__uuidof(ID3D12Resource*),
		&texture.mHandle
	));

	// Create RTV or DSV for attachments
	if (descriptor.usage & TextureUsage_Attachment)
	{
		if (Utils::IsDepthFormat(descriptor.format))
		{
			auto& dsvHeap = static_cast<DirectXDevice*>(this)->mDsvHeap;
			auto index = dsvHeap.heap.Allocate();

			D3D12_CPU_DESCRIPTOR_HANDLE handle = dsvHeap.cpuHandle;
			handle.ptr += (size_t)index.data * (size_t)dsvHeap.size;

			D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
			dsvDesc.Format = resourceDesc.Format;
			dsvDesc.ViewDimension = TextureDimensionToD3D12_DSV_DIMENSION(descriptor.dimension);
			static_cast<ID3D12Device10*>(mHandle)->CreateDepthStencilView(static_cast<ID3D12Resource*>(texture.mHandle), &dsvDesc, handle);
			texture.mView = handle;
		}
		else
		{
			auto& rtvHeap = static_cast<DirectXDevice*>(this)->mRtvHeap;
			auto index = rtvHeap.heap.Allocate();

			D3D12_CPU_DESCRIPTOR_HANDLE handle = rtvHeap.cpuHandle;
			handle.ptr += (size_t)index.data * (size_t)rtvHeap.size;

			D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
			rtvDesc.Format = resourceDesc.Format;
			rtvDesc.ViewDimension = TextureDimensionToD3D12_RTV_DIMENSION(descriptor.dimension);
			if (descriptor.dimension == TextureDimension::TextureCube)
			{
				rtvDesc.Texture2DArray = {
					.MipSlice = 0,
					.FirstArraySlice = 0,
					.ArraySize = 6,
					.PlaneSlice = 0
				};
			}
			static_cast<ID3D12Device10*>(mHandle)->CreateRenderTargetView(static_cast<ID3D12Resource*>(texture.mHandle), &rtvDesc, handle);
			texture.mView = handle;
		}
	}
	
	texture.mResourceView = Utils::IsDepthFormat(descriptor.format) ? InvalidResourceIndex : CreateResourceView(texture);
	return texture;
}

Shader GraphicsDevice::GenerateShader(const TString& entryPoint, ShaderStage stage) const
{
	Shader shader(entryPoint, stage);

	auto shaderFile = GameInstance->GetDefaultAssetPath().append("Shaders/" + entryPoint + ".dxil");
	auto bytecodeLength = IOUtils::QueryFileBufferSize(shaderFile);
	auto bytecode = new uint8_t[bytecodeLength];
	IOUtils::ReadBinaryFile(shaderFile, bytecode, bytecodeLength);

	D3D12_SHADER_BYTECODE* shaderBytecode = new D3D12_SHADER_BYTECODE;
	shaderBytecode->pShaderBytecode = bytecode;
	shaderBytecode->BytecodeLength = bytecodeLength;
	shader.mHandle = shaderBytecode;
	return shader;
}

void GraphicsDevice::Dispose(Heap& heap)
{
	static_cast<ID3D12Heap*>(heap.mHandle)->Release();
	heap.mHandle = nullptr;
}

void GraphicsDevice::Dispose(Buffer& buffer)
{
	ReleaseResourceView(buffer.mResourceView);
	static_cast<ID3D12Resource*>(buffer.mHandle)->Release();
	buffer.mResourceView = InvalidResourceIndex;
	buffer.mContents = nullptr;
	buffer.mHandle = nullptr;
}

void GraphicsDevice::Dispose(Texture& texture)
{
	if (texture.GetDescriptor().usage & TextureUsage_Attachment)
	{
		if (Utils::IsDepthFormat(texture.GetDescriptor().format))
		{
			auto& dsvHeap = static_cast<DirectXDevice*>(this)->mDsvHeap;
			dsvHeap.heap.Release(dsvHeap.GetResourceIndex(texture.GetView()));
		}
		else
		{
			auto& rtvHeap = static_cast<DirectXDevice*>(this)->mRtvHeap;
			rtvHeap.heap.Release(rtvHeap.GetResourceIndex(texture.GetView()));
		}
	}

	ReleaseResourceView(texture.mResourceView);
	static_cast<ID3D12Resource*>(texture.mHandle)->Release();
	texture.mResourceView = InvalidResourceIndex;
	texture.mHandle = nullptr;
}

DirectXDevice::DirectXDevice()
{
	UINT dxgiFactoryFlags = 0;
#ifdef GDEBUG
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&mD3D12Debug))))
	{
		dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;

		mD3D12Debug->EnableDebugLayer();
		mD3D12Debug->SetEnableGPUBasedValidation(true);

		if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&mDXGIDebug))))
		{
			mDXGIDebug->EnableLeakTrackingForThread();
		}
	}
#endif

	DX_CHECK(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&mFactory)));

#ifdef GDEBUG
	if (SUCCEEDED(mFactory->QueryInterface(IID_PPV_ARGS(&mInfoQueue))))
	{
		static void* emitWarning = nullptr;
		DX_CHECK(mInfoQueue->RegisterMessageCallback(DirectXDebugCallback, D3D12_MESSAGE_CALLBACK_FLAG_NONE, emitWarning, &mDebugCallbackCookie));
	}
#endif

	DX_CHECK(mFactory->EnumAdapterByGpuPreference(0, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&mAdapter)));

	DX_CHECK(D3D12CreateDevice(mAdapter, D3D_FEATURE_LEVEL_12_0, __uuidof(ID3D12Device10), &mHandle));

	mDirectQueue = CreateCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);
	mComputeQueue = CreateCommandQueue(D3D12_COMMAND_LIST_TYPE_COMPUTE);
	mCopyQueue = CreateCommandQueue(D3D12_COMMAND_LIST_TYPE_COPY);

	mRtvHeap = CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, 8192);
	mDsvHeap = CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, 8192);
	mCbvSrvUavHeap = CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, CBV_SRV_HEAP_SIZE);

	DirectXPipelineStateManager::Init(this);

	EventDispatcher<WindowResizeEvent>::Subscribe([this](const WindowResizeEvent& e)
	{
		WaitDeviceIdle();
		for (auto& ctx : mFrameContext)
		{
			ReleaseSwapchainBuffer(ctx.drawable);
		}

		mSize.width = static_cast<float>(e.GetWidth());
		mSize.height = static_cast<float>(e.GetHeight());
		mSwapchain->ResizeBuffers(mMaxFramesInFlight, (UINT)mSize.width, (UINT)mSize.height, TextureFormatToDXGI_FORMAT(mFormat), 0);

		for (uint32_t i = 0; i < mMaxFramesInFlight; i++)
		{
			auto& ctx = mFrameContext[i];
			ctx.drawable = GetSwapchainBuffer(i);
		}
		EventDispatcher<RendererResizeEvent>::Publish(RendererResizeEvent(mSize));
	});

	GLEAM_CORE_INFO("DirectX: Graphics device created.");
}

DirectXDevice::~DirectXDevice()
{
	WaitDeviceIdle();

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
		ctx.fence->Release();

		ReleaseSwapchainBuffer(ctx.drawable);
	}
	mFrameContext.clear();

	DirectXPipelineStateManager::Destroy();
	DirectXTransitionManager::Clear();

	mRtvHeap.handle->Release();
	mDsvHeap.handle->Release();
	mCbvSrvUavHeap.handle->Release();

	mDirectQueue->Release();
	mComputeQueue->Release();
	mCopyQueue->Release();

	mSwapchain->Release();
	mFactory->Release();
	mAdapter->Release();
	static_cast<ID3D12Device10*>(mHandle)->Release();

#ifdef GDEBUG
	if (mInfoQueue)
	{
		mInfoQueue->UnregisterMessageCallback(mDebugCallbackCookie);
		mInfoQueue->Release();
	}

	if (mD3D12Debug)
	{
		mD3D12Debug->Release();
	}
	if (mDXGIDebug)
	{
		OutputDebugStringW(L"DXGI Reports living device objects:\n");
		mDXGIDebug->ReportLiveObjects(DXGI_DEBUG_ALL,
			DXGI_DEBUG_RLO_FLAGS(DXGI_DEBUG_RLO_DETAIL | DXGI_DEBUG_RLO_IGNORE_INTERNAL)
		);

		mDXGIDebug->Release();
	}
#endif
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

	DXGI_FORMAT format = DXGI_FORMAT_B8G8R8A8_UNORM;
	mFormat = DXGI_FORMATtoTextureFormat(format);

	if (mSwapchain != nullptr)
	{
		// Destroy old context
        DestroyPooledObjects();
		for (auto& ctx : mFrameContext)
		{
			ReleaseSwapchainBuffer(ctx.drawable);
			ctx.commandPool.Release();
			ctx.fence->Release();
		}
		mFrameContext.clear();
        
		mSwapchain->ResizeBuffers(mMaxFramesInFlight, (UINT)mSize.width, (UINT)mSize.height, format, 0);
	}
	else
	{
		DXGI_SWAP_CHAIN_DESC1 swapchainDesc =
		{
			.Width = (UINT)mSize.width,
			.Height = (UINT)mSize.height,
			.Format = format,
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
			swapchainDesc.Flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
		}

		SDL_Window* window = GameInstance->GetSubsystem<WindowSystem>()->GetSDLWindow();
		HWND hwnd = (HWND)SDL_GetProperty(SDL_GetWindowProperties(window), SDL_PROPERTY_WINDOW_WIN32_HWND_POINTER, NULL);

		IDXGISwapChain1* swapchain1 = nullptr;
		DX_CHECK(mFactory->CreateSwapChainForHwnd(mDirectQueue, hwnd, &swapchainDesc, nullptr, nullptr, &swapchain1));
		DX_CHECK(swapchain1->QueryInterface(IID_PPV_ARGS(&mSwapchain)));
		swapchain1->Release();
	}

	mFrameContext.resize(mMaxFramesInFlight);
    mPooledObjects.resize(mMaxFramesInFlight);
	for (uint32_t i = 0; i < mMaxFramesInFlight; i++)
	{
		auto& ctx = mFrameContext[i];
		ctx.drawable = GetSwapchainBuffer(i);

		// create fence
		DX_CHECK(static_cast<ID3D12Device10*>(mHandle)->CreateFence(
			ctx.frameCount,
			D3D12_FENCE_FLAG_NONE,
			IID_PPV_ARGS(&ctx.fence)
		));
	}
}

DirectXDrawable DirectXDevice::AcquireNextDrawable()
{
	auto idx = mSwapchain->GetCurrentBackBufferIndex();
	auto& ctx = mFrameContext[idx];
	WaitForID3D12Fence(ctx.fence, ctx.frameCount);
	return ctx.drawable;
}

void DirectXDevice::Present(const CommandBuffer* cmd)
{
	auto& ctx = mFrameContext[mCurrentFrameIndex];

	DirectXTransitionManager::TransitionLayout(static_cast<ID3D12GraphicsCommandList7*>(cmd->GetHandle()),
		ctx.drawable.renderTarget, D3D12_RESOURCE_STATE_PRESENT);

	cmd->End();
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
	mCurrentFrameIndex = (mCurrentFrameIndex + 1) % mMaxFramesInFlight;
}

void DirectXDevice::DestroyFrameObjects(uint32_t frameIndex)
{
	mFrameContext[mCurrentFrameIndex].commandPool.Reset();
}

ID3D12GraphicsCommandList7* DirectXDevice::AllocateCommandList(D3D12_COMMAND_LIST_TYPE type)
{
	auto& pool = mFrameContext[mCurrentFrameIndex].commandPool;

	DirectXCommandList commandList{};
	if (pool.freeCommandLists.empty())
	{
		DX_CHECK(static_cast<ID3D12Device10*>(mHandle)->CreateCommandAllocator(type, IID_PPV_ARGS(&commandList.allocator)));
		DX_CHECK(static_cast<ID3D12Device10*>(mHandle)->CreateCommandList1(0, type, D3D12_COMMAND_LIST_FLAG_NONE, IID_PPV_ARGS(&commandList.handle)));
	}
	else
	{
		// TODO: implement searching for appropriate D3D12_COMMAND_LIST_TYPE
		commandList = pool.freeCommandLists.front();
		pool.freeCommandLists.pop_front();
	}
	pool.usedCommandLists.push_back(commandList);

	commandList.allocator->Reset();
	commandList.handle->Reset(commandList.allocator, nullptr);

	return commandList.handle;
}

DirectXDrawable DirectXDevice::GetSwapchainBuffer(uint32_t buffer)
{
	DirectXDrawable drawable{};
	auto index = mRtvHeap.heap.Allocate();
	drawable.view = mRtvHeap.cpuHandle;
	drawable.view.ptr += (size_t)index.data * (size_t)mRtvHeap.size;

	// create swapchain RTV
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc =
	{
		.Format = TextureFormatToDXGI_FORMAT(mFormat),
		.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D
	};

	DX_CHECK(mSwapchain->GetBuffer(buffer, IID_PPV_ARGS(&drawable.renderTarget)));
	static_cast<ID3D12Device10*>(mHandle)->CreateRenderTargetView(drawable.renderTarget, &rtvDesc, drawable.view);
	DirectXTransitionManager::SetLayout(drawable.renderTarget, D3D12_RESOURCE_STATE_PRESENT);

#ifdef GDEBUG
	TStringStream resourceName;
	resourceName << "Swapchain::Drawable_" << buffer;
	drawable.renderTarget->SetName(StringUtils::Convert(resourceName.str()).data());
#endif

	return drawable;
}

void DirectXDevice::ReleaseSwapchainBuffer(DirectXDrawable& drawable)
{
	drawable.renderTarget->Release();
	mRtvHeap.heap.Release(mRtvHeap.GetResourceIndex(drawable.view));
	drawable.renderTarget = nullptr;
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

DirectXDescriptorHeap DirectXDevice::CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type, D3D12_DESCRIPTOR_HEAP_FLAGS flags, UINT capacity) const
{
	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.Type = type;
	desc.Flags = flags;
	desc.NumDescriptors = (type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) ? capacity * 2 : capacity;

	DirectXDescriptorHeap heap{};
	heap.heap = ResourceDescriptorHeap(capacity);
	DX_CHECK(static_cast<ID3D12Device10*>(mHandle)->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&heap.handle)));
	heap.size = static_cast<ID3D12Device10*>(mHandle)->GetDescriptorHandleIncrementSize(type);
	heap.cpuHandle = heap.handle->GetCPUDescriptorHandleForHeapStart();
	if (flags & D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE)
	{
		heap.gpuHandle = heap.handle->GetGPUDescriptorHandleForHeapStart();
	}
	return heap;
}

ShaderResourceIndex DirectXDevice::CreateResourceView(const Buffer& buffer)
{
	auto index = mCbvSrvUavHeap.heap.Allocate();
	D3D12_CPU_DESCRIPTOR_HANDLE handle = mCbvSrvUavHeap.cpuHandle;
	handle.ptr += (size_t)index.data * (size_t)mCbvSrvUavHeap.size;

	// SRV
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_R32_TYPELESS;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Buffer.FirstElement = 0;
	srvDesc.Buffer.NumElements = (UINT)buffer.GetSize() >> 2;
	srvDesc.Buffer.StructureByteStride = 0;
	srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;
	static_cast<ID3D12Device10*>(mHandle)->CreateShaderResourceView(static_cast<ID3D12Resource*>(buffer.GetHandle()), &srvDesc, handle);

	handle.ptr += (UINT64)(mCbvSrvUavHeap.size * CBV_SRV_HEAP_SIZE);

	// UAV
	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.Format = DXGI_FORMAT_R32_TYPELESS;
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
	uavDesc.Buffer.FirstElement = 0;
	uavDesc.Buffer.NumElements = (UINT)buffer.GetSize() >> 2;
	uavDesc.Buffer.StructureByteStride = 0;
	uavDesc.Buffer.CounterOffsetInBytes = 0;
	uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;
	static_cast<ID3D12Device10*>(mHandle)->CreateUnorderedAccessView(static_cast<ID3D12Resource*>(buffer.GetHandle()), nullptr, &uavDesc, handle);

	return index;
}

ShaderResourceIndex DirectXDevice::CreateResourceView(const Texture& texture)
{
	// SRV
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = TextureFormatToDXGI_FORMAT(texture.GetDescriptor().format);
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	// UAV
	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.Format = srvDesc.Format;
	switch (texture.GetDescriptor().dimension)
	{
		case TextureDimension::Texture2D:
		{
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;

			srvDesc.Texture2D = {
				.MostDetailedMip = 0,
				.MipLevels = texture.GetMipMapLevels(),
				.PlaneSlice = 0,
				.ResourceMinLODClamp = 0.0f
			};

			uavDesc.Texture2D = {
				.MipSlice = 0
			};

			break;
		}
		case TextureDimension::TextureCube:
		{
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
			uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;

			srvDesc.TextureCube = {
				.MostDetailedMip = 0,
				.MipLevels = texture.GetMipMapLevels(),
				.ResourceMinLODClamp = 0,
			};

			uavDesc.Texture2DArray = {
				.MipSlice = 0,
				.FirstArraySlice = 0,
				.ArraySize = 6,
				.PlaneSlice = 0
			};

			break;
		}
		default:
		{
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_UNKNOWN;
			uavDesc.ViewDimension = D3D12_UAV_DIMENSION_UNKNOWN;
			break;
		}
	}

	auto index = mCbvSrvUavHeap.heap.Allocate();
	D3D12_CPU_DESCRIPTOR_HANDLE handle = mCbvSrvUavHeap.cpuHandle;
	handle.ptr += (size_t)index.data * (size_t)mCbvSrvUavHeap.size;

	// SRV
	if (texture.GetDescriptor().usage & TextureUsage_Sampled)
	{
		static_cast<ID3D12Device10*>(mHandle)->CreateShaderResourceView(static_cast<ID3D12Resource*>(texture.GetHandle()), &srvDesc, handle);
	}

	// UAV
	if (texture.GetDescriptor().usage & TextureUsage_Storage)
	{
		handle.ptr += (UINT64)(mCbvSrvUavHeap.size * CBV_SRV_HEAP_SIZE);
		static_cast<ID3D12Device10*>(mHandle)->CreateUnorderedAccessView(static_cast<ID3D12Resource*>(texture.GetHandle()), nullptr, &uavDesc, handle);
	}
	return index;
}

void DirectXDevice::ReleaseResourceView(ShaderResourceIndex view)
{
	if (view != InvalidResourceIndex)
	{
		mCbvSrvUavHeap.heap.Release(view);
	}
}

void DirectXDevice::WaitDeviceIdle() const
{
	WaitQueueIdle(mCopyQueue);
	WaitQueueIdle(mComputeQueue);
	WaitQueueIdle(mDirectQueue);
}

void DirectXDevice::WaitQueueIdle(ID3D12CommandQueue* queue) const
{
	constexpr static uint64_t fenceValue = 1;

	ID3D12Fence* fence = nullptr;
	DX_CHECK(static_cast<ID3D12Device10*>(mHandle)->CreateFence(
		fenceValue,
		D3D12_FENCE_FLAG_NONE,
		IID_PPV_ARGS(&fence)
	));

	DX_CHECK(queue->Signal(fence, fenceValue));
	WaitForID3D12Fence(fence, fenceValue);
	fence->Release();
}

DirectXDescriptorHeap& DirectXDevice::GetCbvSrvUavHeap()
{
	return mCbvSrvUavHeap;
}

const DirectXDescriptorHeap& DirectXDevice::GetCbvSrvUavHeap() const
{
	return mCbvSrvUavHeap;
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

void DirectXCommandPool::Reset()
{
	freeCommandLists.insert(freeCommandLists.end(), usedCommandLists.begin(), usedCommandLists.end());
	usedCommandLists.clear();
}

void DirectXCommandPool::Release()
{
	for (auto& cmd : usedCommandLists)
	{
		cmd.handle->Release();
		cmd.allocator->Release();

		cmd.handle = nullptr;
		cmd.allocator = nullptr;
	}

	for (auto& cmd : freeCommandLists)
	{
		cmd.handle->Release();
		cmd.allocator->Release();

		cmd.handle = nullptr;
		cmd.allocator = nullptr;
	}
}

ShaderResourceIndex DirectXDescriptorHeap::GetResourceIndex(D3D12_CPU_DESCRIPTOR_HANDLE view)
{
	ShaderResourceIndex index;
	index.data = (UINT)((view.ptr - handle->GetCPUDescriptorHandleForHeapStart().ptr) / (SIZE_T)size);
	return index;
}

#endif
