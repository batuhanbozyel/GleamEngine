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
	if (Severity & D3D12_MESSAGE_SEVERITY_MESSAGE)
	{
		GLEAM_CORE_TRACE("DirectX: {0}", pDescription);
	}
	else if (Severity & D3D12_MESSAGE_SEVERITY_INFO)
	{
		GLEAM_CORE_INFO("DirectX: {0}", pDescription);
	}
	else if (Severity & D3D12_MESSAGE_SEVERITY_WARNING)
	{
		GLEAM_CORE_WARN("DirectX: {0}", pDescription);
	}
	else // if (Severity & D3D12_MESSAGE_SEVERITY_ERROR || Severity & D3D12_MESSAGE_SEVERITY_CORRUPTION)
	{
		GLEAM_ASSERT(false, "DirectX: {0}", pDescription);
	}
}

void RenderSystem::InitializeBackend()
{
	mSwapchain = new DirectXSwapchain();
	mReleaseQueue = new ResourceReleaseQueue(mSwapchain->GetFramesInFlight());
	mDevice = new DirectXDevice(mSwapchain, mReleaseQueue);
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
	desc.Flags = D3D12_HEAP_FLAG_CREATE_NOT_ZEROED;
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
	texture.mHandle = static_cast<DirectXDevice*>(this)->CreateTexture(allocator, descriptor, D3D12_BARRIER_LAYOUT_UNDEFINED);

	const auto& resourceDesc = static_cast<ID3D12Resource2*>(texture.mHandle)->GetDesc1();
	if (descriptor.usage & TextureUsage_Attachment)
	{
		texture.mView = static_cast<DirectXDevice*>(this)->CreateRenderTargetView(static_cast<ID3D12Resource*>(texture.mHandle), resourceDesc);
		texture.mSliceViews = static_cast<DirectXDevice*>(this)->CreateRenderTargetViews(static_cast<ID3D12Resource*>(texture.mHandle), resourceDesc);
	}
	texture.mResourceView = static_cast<DirectXDevice*>(this)->CreateResourceView(texture);
	texture.mSliceUnorderedAccessViews = static_cast<DirectXDevice*>(this)->CreateUnorderedAccessViews(static_cast<ID3D12Resource*>(texture.mHandle), resourceDesc);
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
	ID3D12Resource* resource = static_cast<DirectXDevice*>(this)->CreateResource(allocation, resourceDesc, D3D12_BARRIER_LAYOUT_UNDEFINED, descriptor.name);
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

BottomLevelAccelerationStructure GraphicsDevice::CreateBLAS(const BLASDescriptor& descriptor)
{
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
		.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS | D3D12_RESOURCE_FLAG_RAYTRACING_ACCELERATION_STRUCTURE
	};
	ID3D12Resource* resource = static_cast<DirectXDevice*>(this)->CreateResource(resourceDesc, D3D12_HEAP_TYPE_DEFAULT, D3D12_BARRIER_LAYOUT_UNDEFINED, descriptor.name);
	return BottomLevelAccelerationStructure(descriptor, resource);
}

TopLevelAccelerationStructure GraphicsDevice::CreateTLAS(const TLASDescriptor& descriptor)
{
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
		.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS | D3D12_RESOURCE_FLAG_RAYTRACING_ACCELERATION_STRUCTURE
	};
	ID3D12Resource* resource = static_cast<DirectXDevice*>(this)->CreateResource(resourceDesc, D3D12_HEAP_TYPE_DEFAULT, D3D12_BARRIER_LAYOUT_UNDEFINED, descriptor.name);

	TopLevelAccelerationStructure tlas(descriptor);
	tlas.mHandle = resource;
	tlas.mResourceView = static_cast<DirectXDevice*>(this)->CreateResourceView(tlas);
	return tlas;
}

Shader GraphicsDevice::CompileShader(const TString& entryPoint, ShaderStage stage)
{
	Shader shader(entryPoint, stage);
	auto shaderPath = Globals::BuiltinAssetsDirectory/"Shaders";
	auto shaderFile = Filesystem::OpenRead(shaderPath.Append(entryPoint + ".dxil"), FileType::Binary);
	auto shaderCode = shaderFile->Read();
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

RayTracingPipeline GraphicsDevice::CompileRayTracingPipeline(const RayTracingPipelineStateDescriptor& pipelineDesc)
{
	RayTracingPipeline pipeline(pipelineDesc);

	TArray<D3D12_DXIL_LIBRARY_DESC> shaders;
	shaders.reserve(pipelineDesc.hitGroups.size() * 3 + pipelineDesc.missEntries.size() + 1 /* ray generation */);
	{
		D3D12_DXIL_LIBRARY_DESC library = {};
		auto shader = CreateShader(pipelineDesc.rayGenerationEntry, ShaderStage::RayGeneration);
		library.DXILLibrary = *static_cast<D3D12_SHADER_BYTECODE*>(shader.GetHandle());
		shaders.emplace_back(library);
	}

	for (const auto& missEntry : pipelineDesc.missEntries)
	{
		if (not missEntry.empty())
		{
			D3D12_DXIL_LIBRARY_DESC library = {};
			auto shader = CreateShader(missEntry, ShaderStage::Miss);
			library.DXILLibrary = *static_cast<D3D12_SHADER_BYTECODE*>(shader.GetHandle());
			shaders.emplace_back(library);
		}
	}

	TArray<D3D12_HIT_GROUP_DESC> hitGroups;
	hitGroups.reserve(pipelineDesc.hitGroups.size());

	TArray<TWString> hitShaderNames;
	hitShaderNames.reserve(pipelineDesc.hitGroups.size() * 4);
	for (const auto& hitGroup : pipelineDesc.hitGroups)
	{
		if (hitGroup.name.empty())
		{
			continue;
		}

		TWString& hitGroupName = hitShaderNames.emplace_back(TWString(hitGroup.name));

		D3D12_HIT_GROUP_DESC d3d12HitGroup = {};
		d3d12HitGroup.Type = D3D12_HIT_GROUP_TYPE_TRIANGLES;
		d3d12HitGroup.HitGroupExport = hitGroupName.c_str();

		if (not hitGroup.closestHitEntry.empty())
		{
			TWString& entryPoint = hitShaderNames.emplace_back(TWString(hitGroup.closestHitEntry));
			d3d12HitGroup.ClosestHitShaderImport = entryPoint.c_str();

			D3D12_DXIL_LIBRARY_DESC library = {};
			auto shader = CreateShader(hitGroup.closestHitEntry, ShaderStage::ClosestHit);
			library.DXILLibrary = *static_cast<D3D12_SHADER_BYTECODE*>(shader.GetHandle());
			shaders.emplace_back(library);
		}

		if (not hitGroup.anyHitEntry.empty())
		{
			TWString& entryPoint = hitShaderNames.emplace_back(TWString(hitGroup.anyHitEntry));
			d3d12HitGroup.AnyHitShaderImport = entryPoint.c_str();

			D3D12_DXIL_LIBRARY_DESC library = {};
			auto shader = CreateShader(hitGroup.anyHitEntry, ShaderStage::AnyHit);
			library.DXILLibrary = *static_cast<D3D12_SHADER_BYTECODE*>(shader.GetHandle());
			shaders.emplace_back(library);
		}

		if (not hitGroup.intersectionEntry.empty())
		{
			d3d12HitGroup.Type = D3D12_HIT_GROUP_TYPE_PROCEDURAL_PRIMITIVE;

			TWString& entryPoint = hitShaderNames.emplace_back(TWString(hitGroup.intersectionEntry));
			d3d12HitGroup.IntersectionShaderImport = entryPoint.c_str();

			D3D12_DXIL_LIBRARY_DESC library = {};
			auto shader = CreateShader(hitGroup.intersectionEntry, ShaderStage::Intersection);
			library.DXILLibrary = *static_cast<D3D12_SHADER_BYTECODE*>(shader.GetHandle());
			shaders.emplace_back(library);
		}
		hitGroups.emplace_back(d3d12HitGroup);
	}

	D3D12_NODE_MASK nodeMask = {};
	nodeMask.NodeMask = 1;

	D3D12_GLOBAL_ROOT_SIGNATURE globalRootSig = {};
	globalRootSig.pGlobalRootSignature = static_cast<DirectXDevice*>(this)->GetGlobalRootSignature();

	D3D12_RAYTRACING_PIPELINE_CONFIG pipelineConfig = {};
	pipelineConfig.MaxTraceRecursionDepth = pipelineDesc.maxRecursionDepth;

	D3D12_RAYTRACING_SHADER_CONFIG shaderConfig = {};
	shaderConfig.MaxAttributeSizeInBytes = pipelineDesc.maxAttributeSize;
	shaderConfig.MaxPayloadSizeInBytes = pipelineDesc.maxPayloadSize;

	TArray<D3D12_STATE_SUBOBJECT> stateObjects;
	stateObjects.reserve(hitGroups.size() + shaders.size() + 4);
	stateObjects.emplace_back(D3D12_STATE_SUBOBJECT{ D3D12_STATE_SUBOBJECT_TYPE_NODE_MASK, &nodeMask });
	stateObjects.emplace_back(D3D12_STATE_SUBOBJECT{ D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE, &globalRootSig });
	stateObjects.emplace_back(D3D12_STATE_SUBOBJECT{ D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG, &pipelineConfig });
	stateObjects.emplace_back(D3D12_STATE_SUBOBJECT{ D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG, &shaderConfig });

	for (const auto& library : shaders)
	{
		stateObjects.emplace_back(D3D12_STATE_SUBOBJECT{ D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY, &library });
	}

	for (const auto& hitGroup : hitGroups)
	{
		stateObjects.emplace_back(D3D12_STATE_SUBOBJECT{ D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP, &hitGroup });
	}

	D3D12_STATE_OBJECT_DESC stateObjectDesc = {};
	stateObjectDesc.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;
	stateObjectDesc.pSubobjects = stateObjects.data();
	stateObjectDesc.NumSubobjects = (UINT)stateObjects.size();
	DX_CHECK(static_cast<ID3D12Device10*>(mHandle)->CreateStateObject(&stateObjectDesc, __uuidof(ID3D12StateObject*), &pipeline.mHandle));

	pipeline.mShaderBindingTable = static_cast<DirectXDevice*>(this)->CreateShaderBindingTable(pipeline);
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

void GraphicsDevice::Dispose(GPUAllocator* allocator, Buffer& buffer, const BarrierState& aliasState)
{
	if (buffer.GetDescriptor().memoryType == MemoryType::CPU)
	{
		ID3D12Resource* resource = static_cast<ID3D12Resource*>(buffer.GetHandle());
		mReleaseQueue->AddResource([this, allocator, resource, view = buffer.mResourceView]()
		{
			const auto& allocation = allocator->GetAllocation(resource);
			allocator->Free(allocation, BarrierState::NonAliased());

			resource->Release();
			static_cast<DirectXDevice*>(this)->ReleaseResourceView(view);
		}, static_cast<Swapchain*>(mSurface)->GetFrameIndex());
	}
	else // if (buffer.GetDescriptor().memoryType == MemoryType::GPU)
	{
		const auto& allocation = allocator->GetAllocation(buffer.GetHandle());
		allocator->Free(allocation, aliasState);

		ID3D12Resource* resource = static_cast<ID3D12Resource*>(buffer.GetHandle());
		mReleaseQueue->AddResource([this, resource, view = buffer.mResourceView]()
		{
			resource->Release();
			static_cast<DirectXDevice*>(this)->ReleaseResourceView(view);
		}, static_cast<Swapchain*>(mSurface)->GetFrameIndex());
	}

	buffer.mResourceView = InvalidResourceIndex;
	buffer.mContents = nullptr;
	buffer.mHandle = nullptr;
}

void GraphicsDevice::Dispose(GPUAllocator* allocator, Texture& texture, const BarrierState& aliasState)
{
	const auto& allocation = allocator->GetAllocation(texture.GetHandle());
	allocator->Free(allocation, aliasState);

	ID3D12Resource* resource = static_cast<ID3D12Resource*>(texture.GetHandle());
	mReleaseQueue->AddResource([this, resource,
		srv = texture.mResourceView,
		rtv = texture.GetRenderTargetView(),
		usage = texture.GetDescriptor().usage,
		format = texture.GetDescriptor().format,
		sliceViews = texture.mSliceViews,
		sliceUnorderedViews = texture.mSliceUnorderedAccessViews]()
	{
		resource->Release();
		static_cast<DirectXDevice*>(this)->ReleaseResourceView(srv);

		// Release main attachment view
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
			
			// Release slice views
			for (const auto& sliceView : sliceViews)
			{
				if (Utils::IsDepthFormat(format))
				{
					auto& dsvHeap = static_cast<DirectXDevice*>(this)->mDsvHeap;
					dsvHeap.heap.Release(dsvHeap.GetResourceIndex(sliceView));
				}
				else
				{
					auto& rtvHeap = static_cast<DirectXDevice*>(this)->mRtvHeap;
					rtvHeap.heap.Release(rtvHeap.GetResourceIndex(sliceView));
				}
			}
		}
		
		// Release slice resource views
		for (const auto& unorderedView : sliceUnorderedViews)
		{
			static_cast<DirectXDevice*>(this)->ReleaseResourceView(unorderedView);
		}
	}, static_cast<Swapchain*>(mSurface)->GetFrameIndex());

	texture.mResourceView = InvalidResourceIndex;
	texture.mHandle = nullptr;
	texture.mView = {};
	texture.mSliceViews.clear();
	texture.mSliceUnorderedAccessViews.clear();
}

void GraphicsDevice::Dispose(BottomLevelAccelerationStructure& blas)
{
	mReleaseQueue->AddResource([this, resource = static_cast<ID3D12Resource*>(blas.GetHandle())]()
	{
		resource->Release();
	}, static_cast<Swapchain*>(mSurface)->GetFrameIndex());
	blas.mHandle = nullptr;
}

void GraphicsDevice::Dispose(TopLevelAccelerationStructure& tlas)
{
	mReleaseQueue->AddResource([this,
								resource = static_cast<ID3D12Resource*>(tlas.GetHandle()),
								srv = tlas.mResourceView]()
	{
		resource->Release();
		static_cast<DirectXDevice*>(this)->ReleaseResourceView(srv);
	}, static_cast<Swapchain*>(mSurface)->GetFrameIndex());

	tlas.mResourceView = InvalidResourceIndex;
	tlas.mHandle = nullptr;
}

void GraphicsDevice::Dispose(Shader& shader)
{
	eastl::erase_if(mShaderCache, [&shader](const auto& cache) -> bool
	{
		return cache.GetHandle() == shader.GetHandle();
	});

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
	mComputePipelineCache.erase(ComputePipelineHandle(pipeline.GetHash()));

	auto resource = static_cast<ID3D12PipelineState*>(pipeline.mHandle);
	mReleaseQueue->AddResource([resource]()
	{
		resource->Release();
	}, static_cast<Swapchain*>(mSurface)->GetFrameIndex());
	pipeline.mHandle = nullptr;
}

void GraphicsDevice::Dispose(GraphicsPipeline& pipeline)
{
	mGraphicsPipelineCache.erase(GraphicsPipelineHandle(pipeline.GetHash()));

	auto resource = static_cast<ID3D12PipelineState*>(pipeline.mHandle);
	mReleaseQueue->AddResource([resource]()
	{
		resource->Release();
	}, static_cast<Swapchain*>(mSurface)->GetFrameIndex());
	pipeline.mHandle = nullptr;
}

void GraphicsDevice::Dispose(RayTracingPipeline& pipeline)
{
	mRayTracingPipelineCache.erase(RayTracingPipelineHandle(pipeline.GetHash()));

	auto resource = static_cast<ID3D12StateObject*>(pipeline.mHandle);
	auto sbt = static_cast<ID3D12Resource*>(pipeline.GetShaderBindingTable().GetHandle());
	mReleaseQueue->AddResource([resource, sbt]()
	{
		sbt->Release();
		resource->Release();
	}, static_cast<Swapchain*>(mSurface)->GetFrameIndex());
	pipeline.mHandle = nullptr;
	pipeline.mShaderBindingTable = {};
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
	}
	DX_CHECK(D3D12CreateDevice(swapchain->mAdapter, D3D_FEATURE_LEVEL_12_0, __uuidof(ID3D12Device10), &mHandle));

	if (Globals::CLI->HasFlag("--debug-layer"))
	{
		if (SUCCEEDED(static_cast<ID3D12Device10*>(mHandle)->QueryInterface(IID_PPV_ARGS(&mInfoQueue))))
		{
			D3D12_MESSAGE_ID denyIds[] = {
					D3D12_MESSAGE_ID_HEAP_ADDRESS_RANGE_INTERSECTS_MULTIPLE_BUFFERS,
			};

			D3D12_INFO_QUEUE_FILTER filter = {};
			filter.DenyList.NumIDs = _countof(denyIds);
			filter.DenyList.pIDList = denyIds;
			DX_CHECK(mInfoQueue->AddStorageFilterEntries(&filter));

			static void* emitWarning = nullptr;
			DX_CHECK(mInfoQueue->RegisterMessageCallback(DirectXDebugCallback, D3D12_MESSAGE_CALLBACK_FLAG_NONE, emitWarning, &mDebugCallbackCookie));
		}
	}

	mDirectQueue = CreateCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);
	mComputeQueue = CreateCommandQueue(D3D12_COMMAND_LIST_TYPE_COMPUTE);
	mCopyQueue = CreateCommandQueue(D3D12_COMMAND_LIST_TYPE_COPY);

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

	for (auto& [_, pipeline] : mRayTracingPipelineCache)
	{
		static_cast<ID3D12Resource*>(pipeline.GetShaderBindingTable().GetHandle())->Release();
		static_cast<ID3D12StateObject*>(pipeline.GetHandle())->Release();
	}
	mRayTracingPipelineCache.clear();

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

	mDirectQueue.Release();
	mComputeQueue.Release();
	mCopyQueue.Release();

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
}

ShaderBindingTable DirectXDevice::CreateShaderBindingTable(const RayTracingPipeline& pipeline)
{
	ID3D12StateObjectProperties* stateObjectProperties = nullptr;
	DX_CHECK(static_cast<ID3D12StateObject*>(pipeline.GetHandle())->QueryInterface(IID_PPV_ARGS(&stateObjectProperties)));

	constexpr uint32_t shaderRecordSize = Math::AlignUp(D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES, D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT);
	constexpr uint32_t rayGenTableSize = Math::AlignUp(shaderRecordSize, (UINT)D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);

	const auto& pipelineDesc = pipeline.GetDescriptor();
	uint32_t missTableSize = Math::AlignUp(static_cast<uint32_t>(pipelineDesc.missEntries.size()) * shaderRecordSize, (UINT)D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);
	uint32_t hitGroupTableSize = Math::AlignUp(static_cast<uint32_t>(pipelineDesc.hitGroups.size()) * shaderRecordSize, (UINT)D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);
	uint32_t totalSize = rayGenTableSize + missTableSize + hitGroupTableSize;

	D3D12_HEAP_PROPERTIES heapProps = {};
	heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

	D3D12_RESOURCE_DESC1 resourceDesc = {
		.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
		.Alignment = 0,
		.Width = totalSize,
		.Height = 1,
		.DepthOrArraySize = 1,
		.MipLevels = 1,
		.Format = DXGI_FORMAT_UNKNOWN,
		.SampleDesc = {.Count = 1, .Quality = 0 },
		.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
		.Flags = D3D12_RESOURCE_FLAG_NONE
	};
	ID3D12Resource* resource = CreateResource(resourceDesc, D3D12_HEAP_TYPE_UPLOAD, D3D12_BARRIER_LAYOUT_UNDEFINED, "ShaderBindingTable");

	void* sbtPtr = nullptr;
	DX_CHECK(resource->Map(0, nullptr, &sbtPtr));

	uint32_t offset = 0;

	// Ray generation record
	{
		TWString entryPoint(pipelineDesc.rayGenerationEntry);
		void* shaderId = stateObjectProperties->GetShaderIdentifier(entryPoint.c_str());
		GLEAM_ASSERT(shaderId, "DirectX: Failed to get ray generation shader identifier.");
		memcpy(OffsetPointer(sbtPtr, offset), shaderId, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
	}
	offset += rayGenTableSize;

	// Miss records
	for (const auto& missEntry : pipelineDesc.missEntries)
	{
		if (not missEntry.empty())
		{
			TWString entryPoint(missEntry);
			void* shaderId = stateObjectProperties->GetShaderIdentifier(entryPoint.c_str());
			GLEAM_ASSERT(shaderId, "DirectX: Failed to get miss shader identifier for: {0}", missEntry);
			memcpy(OffsetPointer(sbtPtr, offset), shaderId, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
		}
		offset += shaderRecordSize;
	}
	offset = rayGenTableSize + missTableSize;

	// Hit group records
	for (const auto& hitGroup : pipelineDesc.hitGroups)
	{
		if (not hitGroup.name.empty())
		{
			TWString hitGroupName(hitGroup.name);
			void* shaderId = stateObjectProperties->GetShaderIdentifier(hitGroupName.c_str());
			GLEAM_ASSERT(shaderId, "DirectX: Failed to get hit group shader identifier for: {0}", hitGroup.name);
			memcpy(OffsetPointer(sbtPtr, offset), shaderId, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
		}
		offset += shaderRecordSize;
	}

	resource->Unmap(0, nullptr);
	stateObjectProperties->Release();

	uint64_t baseAddress = resource->GetGPUVirtualAddress();
	GPUVirtualAddressRange rayGenRecord = { .startAddress = baseAddress, .sizeInBytes = rayGenTableSize };
	GPUVirtualAddressRangeAndStride missRecord = { .startAddress = baseAddress + rayGenTableSize, .sizeInBytes = missTableSize, .strideInBytes = shaderRecordSize };
	GPUVirtualAddressRangeAndStride hitGroupRecord =
		hitGroupTableSize > 0
		? GPUVirtualAddressRangeAndStride {.startAddress = baseAddress + rayGenTableSize + missTableSize, .sizeInBytes = hitGroupTableSize, .strideInBytes = shaderRecordSize }
		: GPUVirtualAddressRangeAndStride{};
	return ShaderBindingTable(resource, rayGenRecord, missRecord, hitGroupRecord);
}

ID3D12Resource* DirectXDevice::CreateResource(const D3D12_RESOURCE_DESC1& desc, D3D12_HEAP_TYPE heapType, D3D12_BARRIER_LAYOUT initialLayout, const TString& name) const
{
	D3D12_HEAP_PROPERTIES heapProps = {};
	heapProps.Type = heapType;

	ID3D12Resource* resource = nullptr;
	DX_CHECK(static_cast<ID3D12Device10*>(mHandle)->CreateCommittedResource3(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		initialLayout,
		nullptr,
		nullptr,
		0,
		nullptr,
		IID_PPV_ARGS(&resource)));
	resource->SetName(StringUtils::Convert(name).c_str());
	return resource;
}

ID3D12Resource* DirectXDevice::CreateResource(const GPUAllocation& allocation, const D3D12_RESOURCE_DESC1& desc, D3D12_BARRIER_LAYOUT initialLayout, const TString& name) const
{
	ID3D12Resource* resource = nullptr;
	DX_CHECK(static_cast<ID3D12Device10*>(mHandle)->CreatePlacedResource2(
		static_cast<ID3D12Heap*>(allocation.block->heap.GetHandle()),
		allocation.offset,
		&desc,
		initialLayout,
		nullptr,
		0,
		nullptr,
		IID_PPV_ARGS(&resource)
	));
	resource->SetName(StringUtils::Convert(name).c_str());
	return resource;
}

DirectXCommandQueue DirectXDevice::CreateCommandQueue(D3D12_COMMAND_LIST_TYPE type) const
{
	D3D12_COMMAND_QUEUE_DESC desc{};
	desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	desc.Type = type;

	DirectXCommandQueue queue = {};
	queue.mDevice = static_cast<ID3D12Device10*>(mHandle);
	queue.mType = type;
	DX_CHECK(static_cast<ID3D12Device10*>(mHandle)->CreateCommandQueue(&desc, IID_PPV_ARGS(&queue.mHandle)));
	DX_CHECK(static_cast<ID3D12Device10*>(mHandle)->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&queue.mFence)));
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

ID3D12Resource* DirectXDevice::CreateTexture(GPUAllocator* allocator, const TextureDescriptor& descriptor, D3D12_BARRIER_LAYOUT initialLayout)
{
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

	uint32_t numSlices = descriptor.dimension == TextureDimension::TextureCube ? 6 * descriptor.depth : descriptor.depth;
	D3D12_RESOURCE_DESC1 resourceDesc = {
		.Dimension = TextureDimensionToD3D12_RESOURCE_DIMENSION(descriptor.dimension),
		.Alignment = 0,
		.Width = (UINT64)descriptor.size.width,
		.Height = (UINT)descriptor.size.height,
		.DepthOrArraySize = (UINT16)numSlices,
		.MipLevels = (UINT16)(descriptor.useMipMap ? Texture::CalculateMipLevels(descriptor.size) : 1),
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
	ID3D12Resource* resource = static_cast<DirectXDevice*>(this)->CreateResource(allocation, resourceDesc, initialLayout, descriptor.name);
	allocator->AddAllocation(resource, allocation);
	return resource;
}

RenderTargetView DirectXDevice::CreateRenderTargetView(ID3D12Resource* resource, const D3D12_RESOURCE_DESC1& descriptor)
{
	if (descriptor.Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL)
	{
		auto& dsvHeap = static_cast<DirectXDevice*>(this)->mDsvHeap;
		auto index = dsvHeap.heap.Allocate();

		D3D12_CPU_DESCRIPTOR_HANDLE handle = dsvHeap.cpuHandle;
		handle.ptr += (size_t)index.data * (size_t)dsvHeap.size;

		D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
		dsvDesc.Format = descriptor.Format;
		if (descriptor.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D && descriptor.DepthOrArraySize == 1)
		{
			dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
			dsvDesc.Texture2D.MipSlice = 0;
		}
		else
		{
			dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
			dsvDesc.Texture2DArray.MipSlice = 0;
			dsvDesc.Texture2DArray.FirstArraySlice = 0;
			dsvDesc.Texture2DArray.ArraySize = descriptor.DepthOrArraySize;
		}
		static_cast<ID3D12Device10*>(mHandle)->CreateDepthStencilView(resource, &dsvDesc, handle);
		return handle;
	}

	if (descriptor.Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET)
	{
		auto& rtvHeap = static_cast<DirectXDevice*>(this)->mRtvHeap;
		auto index = rtvHeap.heap.Allocate();

		D3D12_CPU_DESCRIPTOR_HANDLE handle = rtvHeap.cpuHandle;
		handle.ptr += (size_t)index.data * (size_t)rtvHeap.size;

		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
		rtvDesc.Format = descriptor.Format;
		switch (descriptor.Dimension)
		{
			case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
			{
				if (descriptor.DepthOrArraySize == 1)
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
					rtvDesc.Texture2DArray.ArraySize = descriptor.DepthOrArraySize;
					rtvDesc.Texture2DArray.PlaneSlice = 0;
				}
				break;
			}
			case D3D12_RESOURCE_DIMENSION_TEXTURE3D:
			{
				rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE3D;
				rtvDesc.Texture3D.MipSlice = 0;
				rtvDesc.Texture3D.FirstWSlice = 0;
				rtvDesc.Texture3D.WSize = descriptor.DepthOrArraySize;
				break;
			}
		}
		static_cast<ID3D12Device10*>(mHandle)->CreateRenderTargetView(resource, &rtvDesc, handle);
		return handle;
	}

	return {};
}

TArray<RenderTargetView> DirectXDevice::CreateRenderTargetViews(ID3D12Resource* resource, const D3D12_RESOURCE_DESC1& descriptor)
{
	if (descriptor.Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL)
	{
		TArray<RenderTargetView> views;
		views.resize(descriptor.DepthOrArraySize * descriptor.MipLevels);

		for (uint32_t i = 0; i < views.size(); i++)
		{
			auto& dsvHeap = static_cast<DirectXDevice*>(this)->mDsvHeap;
			auto index = dsvHeap.heap.Allocate();

			uint32_t slice = Texture::GetSlice(i, descriptor.MipLevels);
			uint32_t mip = Texture::GetMip(i, descriptor.MipLevels);

			D3D12_CPU_DESCRIPTOR_HANDLE handle = dsvHeap.cpuHandle;
			handle.ptr += (size_t)index.data * (size_t)dsvHeap.size;

			D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
			dsvDesc.Format = descriptor.Format;
			dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
			dsvDesc.Texture2DArray.MipSlice = mip;
			dsvDesc.Texture2DArray.FirstArraySlice = slice;
			dsvDesc.Texture2DArray.ArraySize = 1;
			static_cast<ID3D12Device10*>(mHandle)->CreateDepthStencilView(resource, &dsvDesc, handle);
			views[i] = handle;
		}
		return views;
	}

	if (descriptor.Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET)
	{
		TArray<RenderTargetView> views;
		views.resize(descriptor.DepthOrArraySize * descriptor.MipLevels);

		for (uint32_t i = 0; i < views.size(); i++)
		{
			auto& rtvHeap = static_cast<DirectXDevice*>(this)->mRtvHeap;
			auto index = rtvHeap.heap.Allocate();

			uint32_t slice = Texture::GetSlice(i, descriptor.MipLevels);
			uint32_t mip = Texture::GetMip(i, descriptor.MipLevels);

			D3D12_CPU_DESCRIPTOR_HANDLE handle = rtvHeap.cpuHandle;
			handle.ptr += (size_t)index.data * (size_t)rtvHeap.size;

			D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
			rtvDesc.Format = descriptor.Format;

			switch (descriptor.Dimension)
			{
				case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
				{
					rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
					rtvDesc.Texture2DArray.MipSlice = mip;
					rtvDesc.Texture2DArray.FirstArraySlice = slice;
					rtvDesc.Texture2DArray.ArraySize = 1;
					rtvDesc.Texture2DArray.PlaneSlice = 0;
					break;
				}
				case D3D12_RESOURCE_DIMENSION_TEXTURE3D:
				{
					rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE3D;
					rtvDesc.Texture3D.MipSlice = mip;
					rtvDesc.Texture3D.FirstWSlice = slice;
					rtvDesc.Texture3D.WSize = 1;
					break;
				}
			}
			static_cast<ID3D12Device10*>(mHandle)->CreateRenderTargetView(resource, &rtvDesc, handle);
			views[i] = handle;
		}
		return views;
	}

	return {};
}

TArray<ShaderResourceIndex> DirectXDevice::CreateUnorderedAccessViews(ID3D12Resource* resource, const D3D12_RESOURCE_DESC1& descriptor)
{
	if (not (descriptor.Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS))
	{
		return {};
	}

	TArray<ShaderResourceIndex> views;
	views.resize(descriptor.DepthOrArraySize * descriptor.MipLevels);

	for (uint32_t i = 0; i < views.size(); ++i)
	{
		uint32_t slice = Texture::GetSlice(i, descriptor.MipLevels);
		uint32_t mip = Texture::GetMip(i, descriptor.MipLevels);

		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
		switch (descriptor.Format)
		{
			case DXGI_FORMAT_D16_UNORM:
			{
				uavDesc.Format = DXGI_FORMAT_R16_UNORM;
				break;
			}
			case DXGI_FORMAT_D32_FLOAT:
			{
				uavDesc.Format = DXGI_FORMAT_R32_FLOAT;
				break;
			}
			case DXGI_FORMAT_D24_UNORM_S8_UINT:
			{
				uavDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
				break;
			}
			default:
			{
				uavDesc.Format = descriptor.Format;
				break;
			}
		}

		switch (descriptor.Dimension)
		{
			case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
			{
				uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
				uavDesc.Texture2DArray = {
					.MipSlice = mip,
					.FirstArraySlice = slice,
					.ArraySize = 1,
					.PlaneSlice = 0,
				};
				break;
			}
			case D3D12_RESOURCE_DIMENSION_TEXTURE3D:
			{
				uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
				uavDesc.Texture3D = {
					.MipSlice = mip,
					.FirstWSlice = slice,
					.WSize = 1,
				};
				break;
			}
		}

		auto uavHandle = static_cast<DirectXDevice*>(this)->mCbvSrvUavHeap.Allocate();
		auto index = static_cast<DirectXDevice*>(this)->mCbvSrvUavHeap.GetResourceIndex(uavHandle);

		uavHandle.ptr += (UINT64)(static_cast<DirectXDevice*>(this)->mCbvSrvUavHeap.size * CBV_SRV_HEAP_SIZE);
		static_cast<ID3D12Device10*>(mHandle)->CreateUnorderedAccessView(resource, nullptr, &uavDesc, uavHandle);
		views[i] = index;
	}
	return views;
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
					.MipSlice = 0,
					.PlaneSlice = 0
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
			if (texture.GetDescriptor().depth == 1)
			{
				srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
				srvDesc.TextureCube = {
					.MostDetailedMip = 0,
					.MipLevels = texture.GetMipMapLevels(),
					.ResourceMinLODClamp = 0.0f
				};
			}
			else
			{
				srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
				srvDesc.TextureCubeArray = {
					.MostDetailedMip = 0,
					.MipLevels = texture.GetMipMapLevels(),
					.First2DArrayFace = 0,
					.NumCubes = texture.GetDescriptor().depth,
					.ResourceMinLODClamp = 0.0f
				};
			}

			uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
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

ShaderResourceIndex DirectXDevice::CreateResourceView(const TopLevelAccelerationStructure& tlas)
{
	auto srvHandle = mCbvSrvUavHeap.Allocate();

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.RaytracingAccelerationStructure.Location = static_cast<ID3D12Resource*>(tlas.GetHandle())->GetGPUVirtualAddress();

	// For AS SRVs the resource parameter must be nullptr;
	// the GPU VA inside the desc is what D3D12 uses.
	static_cast<ID3D12Device10*>(mHandle)->CreateShaderResourceView(nullptr, &srvDesc, srvHandle);

	return mCbvSrvUavHeap.GetResourceIndex(srvHandle);
}

void DirectXDevice::ReleaseResourceView(ShaderResourceIndex view)
{
	if (view != InvalidResourceIndex)
	{
		mCbvSrvUavHeap.heap.Release(view);
	}
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

DirectXCommandQueue& DirectXDevice::GetDirectQueue()
{
	return mDirectQueue;
}

DirectXCommandQueue& DirectXDevice::GetComputeQueue()
{
	return mComputeQueue;
}

DirectXCommandQueue& DirectXDevice::GetCopyQueue()
{
	return mCopyQueue;
}

ID3D12RootSignature* DirectXDevice::GetGlobalRootSignature() const
{
	return mRootSignature;
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
