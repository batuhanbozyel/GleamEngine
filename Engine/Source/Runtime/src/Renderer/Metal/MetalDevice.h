#pragma once
#ifdef USE_METAL_RENDERER
#include "MetalCommandQueue.h"
#include "Renderer/GraphicsDevice.h"

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

@protocol MetalMeshPipeline <MetalPipeline>
@property (nonatomic, strong) id<MTLRenderPipelineState> pipelineState;
@property (nonatomic, strong) id<MTLDepthStencilState> depthStencilState;
@property (nonatomic, assign) MTLSize meshThreadsPerThreadgroup;
@property (nonatomic, assign) MTLSize objectThreadsPerThreadgroup;
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
    id<MTLBuffer> handle;
#ifdef USE_TEXTURE_VIEW_POOL
    id<MTLTextureViewPool> pool;
#else
    NSMutableDictionary<NSNumber*, id<MTLTexture>>* textureViews;
#endif
};

class MetalDevice final : public GraphicsDevice
{
public:
    
    MetalDevice(RenderSurface* surface, ResourceReleaseQueue* releaseQueue);
    
    ~MetalDevice();
    
    id<MTLBuffer> GetSamplerHeap() const;
	
	id<MTLBuffer> GetCbvSrvUavHeap() const;
	
#ifdef USE_TEXTURE_VIEW_POOL
	id<MTLTextureViewPool> GetRtvHeap() const;
#endif
	
	id<MTLResidencySet> GetResidencySet() const;
	
	id<MTL4ArgumentTable> GetArgumentTable() const;
	
	MetalCommandQueue& GetCommandQueue();
	
	IRRootSignature* GetGlobalRootSignature() const;

	ShaderBindingTable CreateShaderBindingTable(const RayTracingPipeline& pipeline);
	
	id<MTLTexture> CreateTexture(GPUAllocator* allocator, const TextureDescriptor& descriptor);

	RenderTargetView CreateRenderTargetView(const Texture& texture);

	TArray<RenderTargetView> CreateRenderTargetViews(const Texture& texture);

	TArray<ShaderResourceIndex> CreateUnorderedAccessViews(const Texture& texture);
	
	ShaderResourceIndex CreateResourceView(const Buffer& buffer);

	ShaderResourceIndex CreateResourceView(const Texture& texture, MTLTextureViewDescriptor* viewDesc);

	AccelerationStructureView CreateResourceView(const TopLevelAccelerationStructure& tlas);

	void ReleaseResourceView(ShaderResourceIndex view);

	void ReleaseResourceView(AccelerationStructureView view);
    
private:

	virtual void Configure(const RendererConfig& config) override;

	MetalDescriptorHeap CreateSamplerHeap(uint32_t capacity) const;
    
    MetalDescriptorHeap CreateDescriptorHeap(uint32_t capacity) const;
	
	id<MTLSamplerState> CreateSampler(const SamplerState& samplerState);

	IRRootSignature* mRootSignature = nullptr;

	MetalCommandQueue mCommandQueue;

	id<MTLResidencySet> mResidencySet{ nil };

	id<MTL4ArgumentTable> mArgumentTable = nil;

	TArray<void*> mStaticSamplers;
    
	MetalDescriptorHeap mSamplerHeap;
    MetalDescriptorHeap mCbvSrvUavHeap;

};

} // namespace Gleam
#endif
