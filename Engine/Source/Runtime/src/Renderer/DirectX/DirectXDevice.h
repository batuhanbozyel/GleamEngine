#pragma once
#ifdef USE_DIRECTX_RENDERER
#include "DirectXCommandQueue.h"
#include "Renderer/GraphicsDevice.h"
#include "Container/Queue.h"

#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3d12sdklayers.h>
#include <dxgidebug.h>

namespace Gleam {

class DirectXSwapchain;

struct DirectXDescriptorHeap
{
	ResourceDescriptorHeap heap;
	ID3D12DescriptorHeap* handle = nullptr;
	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle;
	D3D12_DESCRIPTOR_HEAP_TYPE type;
	UINT size;
	UINT capacity;

	D3D12_CPU_DESCRIPTOR_HANDLE Allocate();

	void Release(D3D12_CPU_DESCRIPTOR_HANDLE handle);

	ShaderResourceIndex GetResourceIndex(D3D12_CPU_DESCRIPTOR_HANDLE view);
};

class DirectXDevice final : public GraphicsDevice
{
	friend class GraphicsDevice;

public:

	DirectXDevice(RenderSurface* surface, ResourceReleaseQueue* releaseQueue);

    ~DirectXDevice();

	DirectXDescriptorHeap& GetRtvHeap();

	DirectXDescriptorHeap& GetDsvHeap();

	DirectXDescriptorHeap& GetCbvSrvUavHeap();

	DirectXDescriptorHeap& GetClearUavHeap();

	DirectXCommandQueue& GetDirectQueue();

	DirectXCommandQueue& GetComputeQueue();

	DirectXCommandQueue& GetCopyQueue();

	ID3D12RootSignature* GetGlobalRootSignature() const;

	ID3D12CommandSignature* GetDrawIndirectCommandSignature() const;

	ID3D12CommandSignature* GetDrawIndexedIndirectCommandSignature() const;

	ID3D12CommandSignature* GetDispatchIndirectCommandSignature() const;

	ID3D12CommandSignature* GetDispatchMeshIndirectCommandSignature() const;

	ID3D12CommandSignature* GetDispatchRaysIndirectCommandSignature() const;

	ShaderBindingTable CreateShaderBindingTable(const RayTracingPipeline& pipeline);

	ID3D12PipelineState* CompileNativeComputePipeline(const TString& shaderName);

	ID3D12Resource* CreateResource(const D3D12_RESOURCE_DESC1& desc, D3D12_HEAP_TYPE heapType, D3D12_BARRIER_LAYOUT initialLayout, const TString& name) const;

	ID3D12Resource* CreateResource(const GPUAllocation& allocation, const D3D12_RESOURCE_DESC1& desc, D3D12_BARRIER_LAYOUT initialLayout, const TString& name) const;

	ShaderResourceIndex CreateResourceView(const Buffer& buffer);

	ShaderResourceIndex CreateResourceView(const Texture& texture);

	ShaderResourceIndex CreateResourceView(const TopLevelAccelerationStructure& tlas);

	void ReleaseResourceView(ShaderResourceIndex view);

private:
	
	virtual void Configure(const RendererConfig& config) override;

	ID3D12Resource* CreateTexture(GPUAllocator* allocator, const TextureDescriptor& descriptor, D3D12_BARRIER_LAYOUT initialLayout);

	RenderTargetView CreateRenderTargetView(ID3D12Resource* resource, const D3D12_RESOURCE_DESC1& descriptor);

	TArray<RenderTargetView> CreateRenderTargetViews(ID3D12Resource* resource, const D3D12_RESOURCE_DESC1& descriptor);

	TArray<ShaderResourceIndex> CreateUnorderedAccessViews(ID3D12Resource* resource, const D3D12_RESOURCE_DESC1& descriptor);

	DirectXCommandQueue CreateCommandQueue(D3D12_COMMAND_LIST_TYPE type) const;

	DirectXDescriptorHeap CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type, D3D12_DESCRIPTOR_HEAP_FLAGS flags, UINT capacity) const;

	ID3D12CommandSignature* CreateIndirectCommandSignature(D3D12_INDIRECT_ARGUMENT_TYPE type, UINT byteStride, const wchar_t* name) const;

	DWORD mDebugCallbackCookie = 0;
	ID3D12InfoQueue1* mInfoQueue = nullptr;
	ID3D12Debug6* mD3D12Debug = nullptr;

	DirectXCommandQueue mDirectQueue;
	DirectXCommandQueue mComputeQueue;
	DirectXCommandQueue mCopyQueue;

	ID3D12RootSignature* mRootSignature = nullptr;

	ID3D12CommandSignature* mDrawIndirectCommandSignature = nullptr;
	ID3D12CommandSignature* mDrawIndexedIndirectCommandSignature = nullptr;
	ID3D12CommandSignature* mDispatchIndirectCommandSignature = nullptr;
	ID3D12CommandSignature* mDispatchMeshIndirectCommandSignature = nullptr;
	ID3D12CommandSignature* mDispatchRaysIndirectCommandSignature = nullptr;

	HashMap<TString, ID3D12PipelineState*> mNativeComputePipelineCache;

	DirectXDescriptorHeap mRtvHeap;
	DirectXDescriptorHeap mDsvHeap;
	DirectXDescriptorHeap mCbvSrvUavHeap;
	DirectXDescriptorHeap mClearUavHeap;

};

} // namespace Gleam
#endif
