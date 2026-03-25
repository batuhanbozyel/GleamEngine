#pragma once
#ifdef USE_METAL_RENDERER
#include "Renderer/GraphicsDevice.h"
#include "Container/Queue.h"

#import <Metal/Metal.h>
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

@protocol MetalRayTracingPipeline <MetalPipeline>
@property (nonatomic, strong) id<MTLComputePipelineState> pipelineState;
@property (nonatomic, strong) id<MTLIntersectionFunctionTable> intersectionFunctionTable;
@property (nonatomic, strong) id<MTLVisibleFunctionTable> visibleFunctionTable;
@end

@protocol MetalFunction <NSObject>
@property(nonatomic, strong) id<MTLFunction> handle;
@end

@protocol MetalComputeFunction <MetalFunction>
@property(nonatomic, assign) MTLSize threadsPerThreadgroup;
@end

@protocol MetalRayTracingFunction <MetalFunction>
@property(nonatomic, assign) IRObject* dxil;
@end

namespace Gleam {

struct Version;
struct RendererConfig;
struct SamplerState;

struct MetalDescriptorHeap
{
    ResourceDescriptorHeap heap;
	id<MTLTextureViewPool> pool;
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
	
	id<MTLTextureViewPool> GetRtvHeap() const;
	
	id<MTLResidencySet> GetResidencySet() const;
	
	id<MTL4ArgumentTable> GetArgumentTable() const;
	
	id<MTL4CommandQueue> GetCommandQueue() const;
	
	id<MTL4CommandBuffer> AllocateCommandBuffer();
	
	IRRootSignature* GetGlobalRootSignature() const;

	ShaderBindingTable CreateShaderBindingTable(const RayTracingPipeline& pipeline);
	
	ShaderResourceIndex CreateResourceView(const Buffer& buffer);

	ShaderResourceIndex CreateResourceView(const Texture& texture, MTLTextureViewDescriptor* viewDesc);

	AccelerationStructureView CreateResourceView(const TopLevelAccelerationStructure& tlas);

	void ReleaseResourceView(ShaderResourceIndex view);

	void ReleaseResourceView(AccelerationStructureView view);
    
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
