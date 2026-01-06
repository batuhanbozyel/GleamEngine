#include "gpch.h"

#ifdef USE_DIRECTX_RENDERER
#include "DirectXDevice.h"
#include "DirectXUtils.h"
#include "DirectXSwapchain.h"

#include "Core/Engine.h"
#include "Core/Globals.h"
#include "Core/WindowSystem.h"
#include "Renderer/SamplerState.h"
#include "Renderer/RenderSystem.h"

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

void RenderSystem::InitializeBackend()
{
	mSwapchain = CreateScope<DirectXSwapchain>();
	mReleaseQueue = CreateScope<ResourceReleaseQueue>(mSwapchain->GetFramesInFlight());

	mDevice = CreateScope<DirectXDevice>(mSwapchain.get(), mReleaseQueue.get());
	mCopyCommandBuffer = CreateScope<CopyCommandBuffer>(mDevice.get());

	mPersistentAllocator = CreateScope<GPUAllocator>(mDevice.get(), GPUAllocatorDescriptor{ .name = "Persistent GPU Allocator" });
	mTransientAllocator = CreateScope<GPUAllocator>(mDevice.get(), GPUAllocatorDescriptor{ .name = "Transient GPU Allocator" });
}

static D3D12_STATIC_SAMPLER_DESC CreateStaticSampler(const SamplerState& samplerState)
{
	D3D12_STATIC_SAMPLER_DESC sampler{};
	sampler.MipLODBias = 0;
	sampler.MaxAnisotropy = 1;
	sampler.MinLOD = 0.0f;
	sampler.MaxLOD = 16.0f;
	sampler.RegisterSpace = 0;
	sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;

	switch (samplerState.filterMode)
	{
		case FilterMode::Point:
		{
			sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
			break;
		}
		case FilterMode::Bilinear:
		{
			sampler.Filter = D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;
			break;
		}
		case FilterMode::Trilinear:
		{
			sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
			break;
		}
		default: GLEAM_ASSERT(false, "DirectX: Filter mode is not supported!") break;
	}

	switch (samplerState.wrapMode)
	{
		case WrapMode::Repeat:
		{
			sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
			sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
			sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
			break;
		}
		case WrapMode::Clamp:
		{
			sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
			sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
			sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
			break;
		}
		case WrapMode::Mirror:
		{
			sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
			sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
			sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
			break;
		}
		case WrapMode::MirrorOnce:
		{
			sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE;
			sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE;
			sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE;
			break;
		}
		default: GLEAM_ASSERT(false, "DirectX: Wrap mode is not supported!") break;
	}

	return sampler;
}

Heap GraphicsDevice::CreateHeap(const HeapDescriptor& descriptor)
{
	ID3D12Heap* handle = nullptr;
	D3D12_HEAP_DESC desc{};
	desc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
	desc.SizeInBytes = descriptor.size;
	desc.Flags = D3D12_HEAP_FLAG_NONE;
	desc.Properties.Type = descriptor.memoryType == MemoryType::CPU ? D3D12_HEAP_TYPE_UPLOAD : D3D12_HEAP_TYPE_DEFAULT;
	DX_CHECK(static_cast<ID3D12Device10*>(mHandle)->CreateHeap(&desc, IID_PPV_ARGS(&handle)));
	handle->SetName(StringUtils::Convert(descriptor.name).c_str());

	Heap heap(descriptor);
	heap.mHandle = handle;
	heap.mDescriptor.size = descriptor.size;
	heap.mAlignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
	return heap;
}

Texture GraphicsDevice::CreateTexture(GPUAllocator* allocator, const TextureDescriptor& descriptor)
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

	if (descriptor.usage & TextureUsage_Storage && Utils::IsColorFormat(descriptor.format))
	{
		flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	}

	D3D12_RESOURCE_DESC1 resourceDesc = {
		.Dimension = TextureDimensionToD3D12_RESOURCE_DIMENSION(descriptor.dimension),
		.Alignment = 0,
		.Width = (UINT64)descriptor.size.width,
		.Height = (UINT)descriptor.size.height,
		.DepthOrArraySize = (UINT16)descriptor.depth,
		.MipLevels = (UINT16)texture.mMipMapLevels,
		.Format = TextureFormatToDXGI_FORMAT(descriptor.format),
		.SampleDesc = {.Count = 1, .Quality = 0 },
		.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
		.Flags = flags
	};

	D3D12_RESOURCE_ALLOCATION_INFO allocationInfo = static_cast<ID3D12Device10*>(mHandle)->GetResourceAllocationInfo2(0, 1, &resourceDesc, nullptr);
	MemoryRequirements memoryRequirements =
	{
		.size = allocationInfo.SizeInBytes,
		.alignment = allocationInfo.Alignment,
		.type = MemoryType::GPU
	};
	GPUAllocation allocation = allocator->Allocate(memoryRequirements);
	ID3D12Resource* resource = static_cast<DirectXDevice*>(this)->CreateResource(allocation, resourceDesc, descriptor.name);
	allocator->AddAllocation(resource, allocation);
	texture.mHandle = resource;

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
			if (descriptor.dimension == TextureDimension::Texture2D && descriptor.depth == 1)
			{
				dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
			}
			else
			{
				dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
			}
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
			switch (descriptor.dimension)
			{
				case TextureDimension::Texture2D:
				{
					if (descriptor.depth == 1)
					{
						rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
						rtvDesc.Texture2D.MipSlice = 0;
						rtvDesc.Texture2D.PlaneSlice = 0;
					}
					else
					{
						rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
						rtvDesc.Texture2DArray.MipSlice = 0;
						rtvDesc.Texture2DArray.FirstArraySlice = 0;
						rtvDesc.Texture2DArray.ArraySize = descriptor.depth;
						rtvDesc.Texture2DArray.PlaneSlice = 0;
					}
					break;
				}
				case TextureDimension::Texture3D:
				{
					rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE3D;
					rtvDesc.Texture3D.MipSlice = 0;
					rtvDesc.Texture3D.FirstWSlice = 0;
					rtvDesc.Texture3D.WSize = descriptor.depth;
					break;
				}
				case TextureDimension::TextureCube:
				{
					rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
					rtvDesc.Texture2DArray.MipSlice = 0;
					rtvDesc.Texture2DArray.FirstArraySlice = 0;
					rtvDesc.Texture2DArray.ArraySize = 6 * descriptor.depth;
					rtvDesc.Texture2DArray.PlaneSlice = 0;
					break;
				}
					
			}
			static_cast<ID3D12Device10*>(mHandle)->CreateRenderTargetView(static_cast<ID3D12Resource*>(texture.mHandle), &rtvDesc, handle);
			texture.mView = handle;
		}
	}
	texture.mResourceView = static_cast<DirectXDevice*>(this)->CreateResourceView(texture);
	return texture;
}

Buffer GraphicsDevice::CreateBuffer(GPUAllocator* allocator, const BufferDescriptor& descriptor)
{
	auto flags = D3D12_RESOURCE_FLAG_NONE;
	if (descriptor.memoryType != MemoryType::CPU)
	{
		flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	}

	D3D12_RESOURCE_DESC1 resourceDesc = {
		.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
		.Alignment = 0,
		.Width = descriptor.size,
		.Height = 1,
		.DepthOrArraySize = 1,
		.MipLevels = 1,
		.Format = DXGI_FORMAT_UNKNOWN,
		.SampleDesc = {.Count = 1, .Quality = 0 },
		.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
		.Flags = flags
	};
	D3D12_RESOURCE_ALLOCATION_INFO allocationInfo = static_cast<ID3D12Device10*>(mHandle)->GetResourceAllocationInfo2(0, 1, &resourceDesc, nullptr);
	MemoryRequirements memoryRequirements =
	{
		.size = allocationInfo.SizeInBytes,
		.alignment = allocationInfo.Alignment,
		.type = descriptor.memoryType
	};
	GPUAllocation allocation = allocator->Allocate(memoryRequirements);
	ID3D12Resource* resource = static_cast<DirectXDevice*>(this)->CreateResource(allocation, resourceDesc, descriptor.name);
	allocator->AddAllocation(resource, allocation);

	void* contents = nullptr;
	if (descriptor.memoryType != MemoryType::GPU)
	{
		DX_CHECK(resource->Map(0, nullptr, &contents));
	}

	Buffer buffer(descriptor);
	buffer.mHandle = resource;
	buffer.mContents = contents;
	buffer.mAlignment = D3D12_RAW_UAV_SRV_BYTE_ALIGNMENT;
	buffer.mResourceView = static_cast<DirectXDevice*>(this)->CreateResourceView(buffer);
	return buffer;
}

Shader GraphicsDevice::CompileShader(const TString& entryPoint, ShaderStage stage)
{
	Shader shader(entryPoint, stage);
	auto shaderPath = Globals::BuiltinAssetsDirectory/"Shaders";
	auto shaderFile = Filesystem::Open(shaderPath.Append(entryPoint + ".dxil"), FileType::Binary);
	auto shaderCode = shaderFile.Read();
	auto bytecodeLength = shaderCode.size();
	auto bytecode = new uint8_t[bytecodeLength];
	memcpy(bytecode, shaderCode.data(), shaderCode.size());

	D3D12_SHADER_BYTECODE* shaderBytecode = new D3D12_SHADER_BYTECODE;
	shaderBytecode->pShaderBytecode = bytecode;
	shaderBytecode->BytecodeLength = bytecodeLength;
	shader.mHandle = shaderBytecode;
	return shader;
}

ComputePipeline GraphicsDevice::CompileComputePipeline(const ComputePipelineStateDescriptor& pipelineDesc)
{
	ComputePipeline pipeline(pipelineDesc);
	auto shader = CreateShader(pipelineDesc.entryPoint, ShaderStage::Compute);

	D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc{};
	psoDesc.pRootSignature = static_cast<DirectXDevice*>(this)->mRootSignature;
	psoDesc.CS = *static_cast<D3D12_SHADER_BYTECODE*>(shader.GetHandle());
	DX_CHECK(static_cast<ID3D12Device10*>(mHandle)->CreateComputePipelineState(&psoDesc, __uuidof(ID3D12PipelineState*), &pipeline.mHandle));

	TStringStream ss;
	ss << "ComputePipeline::" << shader.GetEntryPoint();

	TWString pipelineName = ss.str();
	static_cast<ID3D12PipelineState*>(pipeline.mHandle)->SetName(pipelineName.c_str());

	return pipeline;
}

GraphicsPipeline GraphicsDevice::CompileGraphicsPipeline(const GraphicsPipelineStateDescriptor& pipelineDesc)
{
	GraphicsPipeline pipeline(pipelineDesc);
	auto vertexShader = CreateShader(pipelineDesc.vertexEntry, ShaderStage::Vertex);
	auto fragmentShader = CreateShader(pipelineDesc.fragmentEntry, ShaderStage::Fragment);

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
	psoDesc.pRootSignature = static_cast<DirectXDevice*>(this)->mRootSignature;
	psoDesc.SampleDesc.Count = 1;
	psoDesc.SampleMask = UINT_MAX;

	// Shader stages
	psoDesc.VS = *static_cast<D3D12_SHADER_BYTECODE*>(vertexShader.GetHandle());
	psoDesc.PS = *static_cast<D3D12_SHADER_BYTECODE*>(fragmentShader.GetHandle());

	// Input assembly state
	psoDesc.PrimitiveTopologyType = PrimitiveToplogyToD3D12_PRIMITIVE_TOPOLOGY_TYPE(pipelineDesc.topology);
	psoDesc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;

	// Rasterizer state
	auto& rasterizerState = psoDesc.RasterizerState;
	rasterizerState.FillMode = pipelineDesc.wireframe ? D3D12_FILL_MODE_WIREFRAME : D3D12_FILL_MODE_SOLID;
	rasterizerState.FrontCounterClockwise = FALSE;
	rasterizerState.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
	rasterizerState.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
	rasterizerState.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
	rasterizerState.DepthClipEnable = FALSE;
	rasterizerState.MultisampleEnable = FALSE;
	rasterizerState.AntialiasedLineEnable = FALSE;
	rasterizerState.ForcedSampleCount = 0;
	rasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
	rasterizerState.CullMode = CullModeToD3D12_CULL_MODE(pipelineDesc.cullingMode);

	// Depth-stencil state
	if (Utils::IsDepthFormat(pipelineDesc.depthFormat))
	{
		psoDesc.DSVFormat = TextureFormatToDXGI_FORMAT(pipelineDesc.depthFormat);

		auto& depthStencilState = psoDesc.DepthStencilState;
		depthStencilState.DepthEnable = pipelineDesc.depthState.compareFunction != CompareFunction::Always;
		depthStencilState.DepthFunc = CompareFunctionToD3D12_COMPARISON_FUNC(pipelineDesc.depthState.compareFunction);
		depthStencilState.DepthWriteMask = pipelineDesc.depthState.writeEnabled ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;

		if (Utils::IsDepthStencilFormat(pipelineDesc.depthFormat))
		{
			depthStencilState.StencilEnable = pipelineDesc.stencilState.enabled;
			depthStencilState.StencilReadMask = pipelineDesc.stencilState.readMask;
			depthStencilState.StencilWriteMask = pipelineDesc.stencilState.writeMask;

			D3D12_DEPTH_STENCILOP_DESC stencilOp{};
			stencilOp.StencilFailOp = StencilOpToD3D12_STENCIL_OP(pipelineDesc.stencilState.failOperation);
			stencilOp.StencilPassOp = StencilOpToD3D12_STENCIL_OP(pipelineDesc.stencilState.passOperation);
			stencilOp.StencilDepthFailOp = StencilOpToD3D12_STENCIL_OP(pipelineDesc.stencilState.depthFailOperation);
			stencilOp.StencilFunc = CompareFunctionToD3D12_COMPARISON_FUNC(pipelineDesc.stencilState.compareFunction);

			depthStencilState.BackFace = stencilOp;
			depthStencilState.FrontFace = stencilOp;
		}
	}

	// Blend state
	auto& blendState = psoDesc.BlendState;
	blendState.AlphaToCoverageEnable = pipelineDesc.alphaToCoverage;
	blendState.IndependentBlendEnable = FALSE;

	psoDesc.NumRenderTargets = (UINT)pipelineDesc.colorFormats.size();
	for (uint32_t i = 0; i < pipelineDesc.colorFormats.size(); i++)
	{
		psoDesc.RTVFormats[i] = TextureFormatToDXGI_FORMAT(pipelineDesc.colorFormats[i]);
		blendState.RenderTarget[i].BlendEnable = pipelineDesc.blendState.enabled;
		blendState.RenderTarget[i].SrcBlend = BlendModeToD3D12_BLEND(pipelineDesc.blendState.sourceColorBlendMode);
		blendState.RenderTarget[i].DestBlend = BlendModeToD3D12_BLEND(pipelineDesc.blendState.destinationColorBlendMode);
		blendState.RenderTarget[i].SrcBlendAlpha = BlendModeToD3D12_BLEND(pipelineDesc.blendState.sourceAlphaBlendMode);
		blendState.RenderTarget[i].DestBlendAlpha = BlendModeToD3D12_BLEND(pipelineDesc.blendState.destinationAlphaBlendMode);
		blendState.RenderTarget[i].BlendOp = BlendOpToD3D12_BLEND_OP(pipelineDesc.blendState.colorBlendOperation);
		blendState.RenderTarget[i].BlendOpAlpha = BlendOpToD3D12_BLEND_OP(pipelineDesc.blendState.alphaBlendOperation);
		blendState.RenderTarget[i].RenderTargetWriteMask = ColorWriteMaskToD3D12_COLOR_WRITE_ENABLE(pipelineDesc.blendState.writeMask);
	}
	DX_CHECK(static_cast<ID3D12Device10*>(mHandle)->CreateGraphicsPipelineState(&psoDesc, __uuidof(ID3D12PipelineState*), &pipeline.mHandle));

	TStringStream ss;
	ss << "GraphicsPipeline::" << vertexShader.GetEntryPoint() << "_" << fragmentShader.GetEntryPoint();

	TWString pipelineName = ss.str();
	static_cast<ID3D12PipelineState*>(pipeline.mHandle)->SetName(pipelineName.c_str());

	return pipeline;
}

void GraphicsDevice::Dispose(Heap& heap)
{
	ID3D12Heap* resource = static_cast<ID3D12Heap*>(heap.GetHandle());
	mReleaseQueue->AddResource([resource]()
	{
		resource->Release();
	}, static_cast<Swapchain*>(mSurface)->GetFrameIndex());
	heap.mHandle = nullptr;
}

void GraphicsDevice::Dispose(GPUAllocator* allocator, Buffer& buffer)
{
	const auto& allocation = allocator->GetAllocation(buffer.GetHandle());
	allocator->Free(allocation);

	ID3D12Resource* resource = static_cast<ID3D12Resource*>(buffer.GetHandle());
	ShaderResourceIndex view = buffer.mResourceView;
	mReleaseQueue->AddResource([this, resource, view]()
	{
		resource->Release();
		static_cast<DirectXDevice*>(this)->ReleaseResourceView(view);
	}, static_cast<Swapchain*>(mSurface)->GetFrameIndex());

	buffer.mResourceView = InvalidResourceIndex;
	buffer.mContents = nullptr;
	buffer.mHandle = nullptr;
}

void GraphicsDevice::Dispose(GPUAllocator* allocator, Texture& texture)
{
	const auto& allocation = allocator->GetAllocation(texture.GetHandle());
	allocator->Free(allocation);

	TextureUsageFlagBits usage = texture.GetDescriptor().usage;
	TextureFormat format = texture.GetDescriptor().format;

	ID3D12Resource* resource = static_cast<ID3D12Resource*>(texture.GetHandle());
	ShaderResourceIndex srv = texture.mResourceView;
	RenderTargetView rtv = texture.GetRenderTargetView();
	mReleaseQueue->AddResource([this, resource, srv, rtv, usage, format]()
	{
		resource->Release();
		static_cast<DirectXDevice*>(this)->ReleaseResourceView(srv);

		if (usage & TextureUsage_Attachment)
		{
			if (Utils::IsDepthFormat(format))
			{
				auto& dsvHeap = static_cast<DirectXDevice*>(this)->mDsvHeap;
				dsvHeap.heap.Release(dsvHeap.GetResourceIndex(rtv));
			}
			else
			{
				auto& rtvHeap = static_cast<DirectXDevice*>(this)->mRtvHeap;
				rtvHeap.heap.Release(rtvHeap.GetResourceIndex(rtv));
			}
		}
	}, static_cast<Swapchain*>(mSurface)->GetFrameIndex());

	texture.mResourceView = InvalidResourceIndex;
	texture.mHandle = nullptr;
	texture.mView = {};
}

void GraphicsDevice::Dispose(Shader& shader)
{
	auto bytecode = static_cast<D3D12_SHADER_BYTECODE*>(shader.mHandle);
	mReleaseQueue->AddResource([bytecode]()
	{
		delete bytecode->pShaderBytecode;
		delete bytecode;
	}, static_cast<Swapchain*>(mSurface)->GetFrameIndex());
	shader.mHandle = nullptr;
}

void GraphicsDevice::Dispose(ComputePipeline& pipeline)
{
	auto resource = static_cast<ID3D12PipelineState*>(pipeline.mHandle);
	mReleaseQueue->AddResource([resource]()
	{
		resource->Release();
	}, static_cast<Swapchain*>(mSurface)->GetFrameIndex());
	pipeline.mHandle = nullptr;
}

void GraphicsDevice::Dispose(GraphicsPipeline& pipeline)
{
	auto resource = static_cast<ID3D12PipelineState*>(pipeline.mHandle);
	mReleaseQueue->AddResource([resource]()
	{
		resource->Release();
	}, static_cast<Swapchain*>(mSurface)->GetFrameIndex());
	pipeline.mHandle = nullptr;
}

DirectXDevice::DirectXDevice(RenderSurface* surface, ResourceReleaseQueue* releaseQueue)
	: GraphicsDevice(surface, releaseQueue)
{
	auto swapchain = static_cast<DirectXSwapchain*>(mSurface);
	if (Globals::CLI->HasFlag("--debug-layer"))
	{
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&mD3D12Debug))))
		{
			mD3D12Debug->EnableDebugLayer();
			if (Globals::CLI->HasFlag("--gpu-based-validation"))
			{
				mD3D12Debug->SetEnableGPUBasedValidation(true);
			}
		}

		if (SUCCEEDED(swapchain->mFactory->QueryInterface(IID_PPV_ARGS(&mInfoQueue))))
		{
			static void* emitWarning = nullptr;
			DX_CHECK(mInfoQueue->RegisterMessageCallback(DirectXDebugCallback, D3D12_MESSAGE_CALLBACK_FLAG_NONE, emitWarning, &mDebugCallbackCookie));
		}
	}
	DX_CHECK(D3D12CreateDevice(swapchain->mAdapter, D3D_FEATURE_LEVEL_12_0, __uuidof(ID3D12Device10), &mHandle));

	mDirectQueue = CreateCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);
	mComputeQueue = CreateCommandQueue(D3D12_COMMAND_LIST_TYPE_COMPUTE);

	mRtvHeap = CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, 8192);
	mDsvHeap = CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, 8192);
	mCbvSrvUavHeap = CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, CBV_SRV_HEAP_SIZE);

	// Static samplers
	auto samplerSates = SamplerState::GetStaticSamplers();
	TArray<D3D12_STATIC_SAMPLER_DESC, samplerSates.size()> staticSamplerDescs{};
	for (uint32_t i = 0; i < samplerSates.size(); i++)
	{
		const auto& sampler = samplerSates[i];
		staticSamplerDescs[i] = CreateStaticSampler(sampler);
		staticSamplerDescs[i].ShaderRegister = i;
	}

	// Root signature
	constexpr uint32_t NumRootParams = PUSH_CONSTANT_SLOT + 1;
	D3D12_ROOT_PARAMETER1 rootSigParams[NumRootParams] = {};
	for (uint32_t i = 0; i < PUSH_CONSTANT_SLOT; i++)
	{
		rootSigParams[i] = {
		  .ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV,
		  .Descriptor = {
			  .ShaderRegister = i,
			  .RegisterSpace = 0,
			  .Flags = D3D12_ROOT_DESCRIPTOR_FLAG_DATA_VOLATILE
		  },
		  .ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL
		};
	}
	// Push constant
	rootSigParams[PUSH_CONSTANT_SLOT] = {
	  .ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS,
	  .Constants = {
		  .ShaderRegister = PUSH_CONSTANT_REGISTER,
		  .RegisterSpace = 0,
		  .Num32BitValues = PUSH_CONSTANT_SIZE / sizeof(uint32_t)
	  },
	  .ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL
	};

	D3D12_VERSIONED_ROOT_SIGNATURE_DESC rootSignature = {};
	rootSignature.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
	rootSignature.Desc_1_1.NumParameters = NumRootParams;
	rootSignature.Desc_1_1.pParameters = rootSigParams;
	rootSignature.Desc_1_1.NumStaticSamplers = (UINT)staticSamplerDescs.size();
	rootSignature.Desc_1_1.pStaticSamplers = staticSamplerDescs.data();
	rootSignature.Desc_1_1.Flags =
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS | D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS
		| D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS
		| D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED;

	ID3DBlob* blob = nullptr;
	ID3DBlob* error = nullptr;
	DX_CHECK(D3D12SerializeVersionedRootSignature(&rootSignature, &blob, &error));

	if (error)
	{
		char* error_msg = (char*)error->GetBufferPointer();
		GLEAM_ASSERT(false, "DirectX: Root signature error: {0}\n", error_msg);
	}

	DX_CHECK(static_cast<ID3D12Device10*>(mHandle)->CreateRootSignature(0, blob->GetBufferPointer(), blob->GetBufferSize(), IID_PPV_ARGS(&mRootSignature)));
	blob->Release();
	mRootSignature->SetName(L"GlobalRootSignature");

	GLEAM_CORE_INFO("DirectX: Graphics device created.");
}

DirectXDevice::~DirectXDevice()
{
	WaitDeviceIdle();

	for (auto& ctx : mFrameContext)
	{
		for (auto& pool : ctx.commandPools)
		{
			pool.Release();
		}
	}
	mFrameContext.clear();

	for (auto& [_, pipeline] : mComputePipelineCache)
	{
		static_cast<ID3D12PipelineState*>(pipeline.GetHandle())->Release();
	}
	mComputePipelineCache.clear();

	for (auto& [_, pipeline] : mGraphicsPipelineCache)
	{
		static_cast<ID3D12PipelineState*>(pipeline.GetHandle())->Release();
	}
	mGraphicsPipelineCache.clear();

	for (auto& shader : mShaderCache)
	{
		auto bytecode = static_cast<D3D12_SHADER_BYTECODE*>(shader.GetHandle());
		delete bytecode->pShaderBytecode;
		delete bytecode;
	}
	mShaderCache.clear();

	mRtvHeap.handle->Release();
	mDsvHeap.handle->Release();
	mCbvSrvUavHeap.handle->Release();

	mDirectQueue->Release();
	mComputeQueue->Release();

	mRootSignature->Release();
	static_cast<ID3D12Device10*>(mHandle)->Release();

	if (mInfoQueue)
	{
		mInfoQueue->UnregisterMessageCallback(mDebugCallbackCookie);
		mInfoQueue->Release();
	}

	if (mD3D12Debug)
	{
		mD3D12Debug->Release();
	}
	GLEAM_CORE_INFO("DirectX: Graphics device destroyed.");
}

void DirectXDevice::Configure(const RendererConfig& config)
{
	auto swapchain = static_cast<DirectXSwapchain*>(mSurface);
	swapchain->Configure(this, config);
	
	for (auto& ctx : mFrameContext)
	{
		for (auto& pool : ctx.commandPools)
		{
			pool.Release();
		}
	}
	mFrameContext.clear();

	mFrameContext.resize(swapchain->mMaxFramesInFlight);
	for (uint32_t i = 0; i < swapchain->mMaxFramesInFlight; i++)
	{
		auto& ctx = mFrameContext[i];
		{
			auto& pool = ctx.commandPools.emplace_back();
			DX_CHECK(static_cast<ID3D12Device10*>(mHandle)->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&pool.allocator)));

			TStringStream ss;
			ss << ID3D12CommandListTypeToString(D3D12_COMMAND_LIST_TYPE_DIRECT) << swapchain->mCurrentFrameIndex;
			TWString cmdAllocatorName = ss.str();
			pool.allocator->SetName(cmdAllocatorName.c_str());
		}

		{
			auto& pool = ctx.commandPools.emplace_back();
			DX_CHECK(static_cast<ID3D12Device10*>(mHandle)->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COMPUTE, IID_PPV_ARGS(&pool.allocator)));

			TStringStream ss;
			ss << ID3D12CommandListTypeToString(D3D12_COMMAND_LIST_TYPE_COMPUTE) << swapchain->mCurrentFrameIndex;
			TWString cmdAllocatorName = ss.str();
			pool.allocator->SetName(cmdAllocatorName.c_str());
		}

		{
			auto& pool = ctx.commandPools.emplace_back();
			DX_CHECK(static_cast<ID3D12Device10*>(mHandle)->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COPY, IID_PPV_ARGS(&pool.allocator)));

			TStringStream ss;
			ss << ID3D12CommandListTypeToString(D3D12_COMMAND_LIST_TYPE_COPY) << swapchain->mCurrentFrameIndex;
			TWString cmdAllocatorName = ss.str();
			pool.allocator->SetName(cmdAllocatorName.c_str());
		}
	}
}

void DirectXDevice::ResetCommandPools(uint32_t frameIndex)
{
	for (auto& pool : mFrameContext[frameIndex].commandPools)
	{
		pool.Reset();
	}
}

ID3D12Resource* DirectXDevice::CreateResource(const GPUAllocation& allocation, const D3D12_RESOURCE_DESC1& desc, const TString& name) const
{
	ID3D12Resource* resource = nullptr;
	DX_CHECK(static_cast<ID3D12Device10*>(mHandle)->CreatePlacedResource2(
		static_cast<ID3D12Heap*>(allocation.block->heap.GetHandle()),
		allocation.offset,
		&desc,
		D3D12_BARRIER_LAYOUT_UNDEFINED,
		nullptr,
		0,
		nullptr,
		IID_PPV_ARGS(&resource)
	));
	resource->SetName(StringUtils::Convert(name).c_str());
	return resource;
}

ID3D12GraphicsCommandList7* DirectXDevice::AllocateCommandList(D3D12_COMMAND_LIST_TYPE type, const TWStringView debugName)
{
	auto swapchain = static_cast<DirectXSwapchain*>(mSurface);

	TStringStream ss;
	ss << ID3D12CommandListTypeToString(type) << swapchain->mCurrentFrameIndex;
	TWString cmdAllocatorName = ss.str();

	for (auto& pool : mFrameContext[swapchain->mCurrentFrameIndex].commandPools)
	{
		if (pool.type == type)
		{
			ID3D12GraphicsCommandList7* commandList = nullptr;
			if (pool.freeCommandLists.empty())
			{
				DX_CHECK(static_cast<ID3D12Device10*>(mHandle)->CreateCommandList1(0, type, D3D12_COMMAND_LIST_FLAG_NONE, IID_PPV_ARGS(&commandList)));
			}
			else
			{
				commandList = pool.freeCommandLists.front();
				pool.freeCommandLists.pop_front();
			}

			pool.usedCommandLists.push_back(commandList);
			DX_CHECK(commandList->Reset(pool.allocator, nullptr));
			commandList->SetName(debugName.data());
			return commandList;
		}
	}

	auto& pool = mFrameContext[swapchain->mCurrentFrameIndex].commandPools.emplace_back();
	DX_CHECK(static_cast<ID3D12Device10*>(mHandle)->CreateCommandAllocator(type, IID_PPV_ARGS(&pool.allocator)));
	pool.allocator->SetName(cmdAllocatorName.c_str());

	ID3D12GraphicsCommandList7* commandList = nullptr;
	DX_CHECK(static_cast<ID3D12Device10*>(mHandle)->CreateCommandList1(0, type, D3D12_COMMAND_LIST_FLAG_NONE, IID_PPV_ARGS(&commandList)));
	pool.usedCommandLists.push_back(commandList);
	DX_CHECK(commandList->Reset(pool.allocator, nullptr));
	commandList->SetName(debugName.data());
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
	if (buffer.GetContents() == nullptr)
	{
		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
		uavDesc.Format = DXGI_FORMAT_R32_TYPELESS;
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
		uavDesc.Buffer.FirstElement = 0;
		uavDesc.Buffer.NumElements = (UINT)buffer.GetSize() >> 2;
		uavDesc.Buffer.StructureByteStride = 0;
		uavDesc.Buffer.CounterOffsetInBytes = 0;
		uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;
		static_cast<ID3D12Device10*>(mHandle)->CreateUnorderedAccessView(static_cast<ID3D12Resource*>(buffer.GetHandle()), nullptr, &uavDesc, handle);
	}
	return index;
}

ShaderResourceIndex DirectXDevice::CreateResourceView(const Texture& texture)
{
	// SRV
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	if (Utils::IsDepthFormat(texture.GetDescriptor().format))
	{
		switch (texture.GetDescriptor().format)
		{
			case TextureFormat::D16_UNorm:
			{
				srvDesc.Format = DXGI_FORMAT_R16_UNORM;
				break;
			}
			case TextureFormat::D32_SFloat:
			{
				srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
				break;
			}
			case TextureFormat::D24_UNorm_S8_UInt:
			{
				srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
				break;
			}
			default: return InvalidResourceIndex;
		}
	}
	else
	{
		srvDesc.Format = TextureFormatToDXGI_FORMAT(texture.GetDescriptor().format);
	}
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	// UAV
	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.Format = srvDesc.Format;

	switch (texture.GetDescriptor().dimension)
	{
		case TextureDimension::Texture2D:
		{
			if (texture.GetDescriptor().depth == 1)
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
			}
			else
			{
				srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
				uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;

				srvDesc.Texture2DArray = {
					.MostDetailedMip = 0,
					.MipLevels = texture.GetMipMapLevels(),
					.FirstArraySlice = 0,
					.ArraySize = texture.GetDescriptor().depth,
					.PlaneSlice = 0,
					.ResourceMinLODClamp = 0.0f
				};

				uavDesc.Texture2DArray = {
					.MipSlice = 0,
					.FirstArraySlice = 0,
					.ArraySize = texture.GetDescriptor().depth,
					.PlaneSlice = 0,
				};
			}
			break;
		}
		case TextureDimension::Texture3D:
		{
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
			uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;

			srvDesc.Texture3D = {
				.MostDetailedMip = 0,
				.MipLevels = texture.GetMipMapLevels(),
				.ResourceMinLODClamp = 0,
			};

			uavDesc.Texture3D = {
				.MipSlice = 0,
				.FirstWSlice = 0,
				.WSize = texture.GetDescriptor().depth,
			};
			break;
		}
		case TextureDimension::TextureCube:
		{
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
			uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;

			srvDesc.Texture2DArray = {
				.MostDetailedMip = 0,
				.MipLevels = texture.GetMipMapLevels(),
				.FirstArraySlice = 0,
				.ArraySize = 6 * texture.GetDescriptor().depth,
				.PlaneSlice = 0,
				.ResourceMinLODClamp = 0.0f
			};

			uavDesc.Texture2DArray = {
				.MipSlice = 0,
				.FirstArraySlice = 0,
				.ArraySize = 6 * texture.GetDescriptor().depth,
				.PlaneSlice = 0,
			};
			break;
		}
	}

	auto srvHandle = mCbvSrvUavHeap.Allocate();

	// SRV
	if (texture.GetDescriptor().usage & TextureUsage_Sampled)
	{
		static_cast<ID3D12Device10*>(mHandle)->CreateShaderResourceView(static_cast<ID3D12Resource*>(texture.GetHandle()), &srvDesc, srvHandle);
	}

	// UAV
	if (texture.GetDescriptor().usage & TextureUsage_Storage && Utils::IsColorFormat(texture.GetDescriptor().format))
	{
		auto uavHandle = srvHandle;
		uavHandle.ptr += (UINT64)(mCbvSrvUavHeap.size * CBV_SRV_HEAP_SIZE);
		static_cast<ID3D12Device10*>(mHandle)->CreateUnorderedAccessView(static_cast<ID3D12Resource*>(texture.GetHandle()), nullptr, &uavDesc, uavHandle);
	}
	return mCbvSrvUavHeap.GetResourceIndex(srvHandle);
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
	WaitQueueIdle(mComputeQueue);
	WaitQueueIdle(mDirectQueue);
}

void DirectXDevice::WaitQueueIdle(ID3D12CommandQueue* queue) const
{
	constexpr static uint64_t fenceValue = 1;

	ID3D12Fence* fence = nullptr;
	DX_CHECK(static_cast<ID3D12Device10*>(mHandle)->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));
	DX_CHECK(queue->Signal(fence, fenceValue));
	WaitForID3D12Fence(fence, fenceValue);
	fence->Release();
}

DirectXDescriptorHeap& DirectXDevice::GetRtvHeap()
{
	return mRtvHeap;
}

DirectXDescriptorHeap& DirectXDevice::GetDsvHeap()
{
	return mDsvHeap;
}

DirectXDescriptorHeap& DirectXDevice::GetCbvSrvUavHeap()
{
	return mCbvSrvUavHeap;
}

const DirectXDescriptorHeap& DirectXDevice::GetRtvHeap() const
{
	return mRtvHeap;
}

const DirectXDescriptorHeap& DirectXDevice::GetDsvHeap() const
{
	return mDsvHeap;
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

ID3D12RootSignature* DirectXDevice::GetGlobalRootSignature() const
{
	return mRootSignature;
}

void DirectXCommandPool::Reset()
{
	freeCommandLists.insert(freeCommandLists.end(), usedCommandLists.begin(), usedCommandLists.end());
	usedCommandLists.clear();
	DX_CHECK(allocator->Reset());
}

void DirectXCommandPool::Release()
{
	for (auto cmdList : usedCommandLists)
	{
		cmdList->Release();
	}
	usedCommandLists.clear();

	for (auto cmdList : freeCommandLists)
	{
		cmdList->Release();
	}
	freeCommandLists.clear();

	allocator->Release();
	allocator = nullptr;
}

D3D12_CPU_DESCRIPTOR_HANDLE DirectXDescriptorHeap::Allocate()
{
	auto index = heap.Allocate();
	auto view = cpuHandle;
	view.ptr += (size_t)index.data * (size_t)size;
	return view;
}

void DirectXDescriptorHeap::Release(D3D12_CPU_DESCRIPTOR_HANDLE handle)
{
	heap.Release(GetResourceIndex(handle));
}

ShaderResourceIndex DirectXDescriptorHeap::GetResourceIndex(D3D12_CPU_DESCRIPTOR_HANDLE view)
{
	ShaderResourceIndex index;
	index.data = (UINT)((view.ptr - handle->GetCPUDescriptorHandleForHeapStart().ptr) / (SIZE_T)size);
	return index;
}

#endif
