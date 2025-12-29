#pragma once
#ifdef USE_METAL_RENDERER
#include "Renderer/GraphicsDevice.h"
#include "Container/Queue.h"

#include <Metal/Metal.h>
#include <metal_irconverter/metal_irconverter.h>

@protocol MetalPipeline <NSObject>
@end

@protocol MetalComputePipeline <MetalPipeline>
@property(nonatomic, strong) id<MTLComputePipelineState> pipelineState;
@property(nonatomic, assign) MTLSize threadsPerThreadgroup;
@end

@protocol MetalGraphicsPipeline <MetalPipeline>
@property (nonatomic, strong) id<MTLRenderPipelineState> pipelineState;
@property (nonatomic, strong) id<MTLDepthStencilState> depthStencilState;
@property (nonatomic, assign) MTLPrimitiveType topology;
@end

@protocol MetalFunction <NSObject>
@property(nonatomic, strong) id<MTLFunction> function;
@end

@protocol MetalComputeFunction <MetalFunction>
@property(nonatomic, assign) MTLSize threadsPerThreadgroup;
@end

namespace Gleam {

struct Version;
struct RendererConfig;
struct SamplerState;

struct MetalDescriptorHeap
{
    ResourceDescriptorHeap heap;
    id<MTLBuffer> handle;
};

struct MetalCommandPool
{
	Deque<void*> usedCommandBuffers;
	Deque<void*> freeCommandBuffers;
	id<MTL4CommandAllocator> allocator;

	void Reset();
	void Release();
};

class MetalDevice final : public GraphicsDevice
{
public:
    
    MetalDevice(RenderSurface* surface, ResourceReleaseQueue* releaseQueue);
    
    ~MetalDevice();
    
    id<MTLBuffer> GetSamplerHeap() const;
	
	id<MTLBuffer> GetCbvSrvUavHeap() const;
	
	id<MTLResidencySet> GetResidencySet() const;
	
	id<MTL4ArgumentTable> GetArgumentTable() const;
	
	id<MTL4CommandQueue> GetCommandQueue() const;
	
	id<MTL4CommandBuffer> AllocateCommandBuffer();
	
	IRRootSignature* GetGlobalRootSignature() const;
	
	ShaderResourceIndex CreateResourceView(const Buffer& buffer);

	ShaderResourceIndex CreateResourceView(const Texture& texture);

	void ReleaseResourceView(ShaderResourceIndex view);
    
private:

	virtual void Configure(const RendererConfig& config) override;
	
	virtual void ResetCommandPools(uint32_t frameIdx) override;
	
	MetalDescriptorHeap CreateSamplerHeap(uint32_t capacity) const;
    
    MetalDescriptorHeap CreateDescriptorHeap(uint32_t capacity) const;
	
	id<MTLSamplerState> CreateSampler(const SamplerState& samplerState);

	IRRootSignature* mRootSignature = nullptr;
	
    id<MTL4CommandQueue> mCommandQueue{ nil };
	
	id<MTLResidencySet> mResidencySet{ nil };
	
	id<MTL4ArgumentTable> mArgumentTable = nil;
	
	TArray<MetalCommandPool> mCommandPools;
	
	TArray<void*> mStaticSamplers;
    
	MetalDescriptorHeap mSamplerHeap;
	
    MetalDescriptorHeap mCbvSrvUavHeap;

};

} // namespace Gleam
#endif
