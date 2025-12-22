#pragma once
#ifdef USE_METAL_RENDERER
#include "Renderer/GraphicsDevice.h"

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

struct MetalDescriptorHeap
{
    ResourceDescriptorHeap heap;
    id<MTLBuffer> handle;
};

class MetalDevice final : public GraphicsDevice
{
public:
    
    MetalDevice(RenderSurface* surface, ResourceReleaseQueue* releaseQueue);
    
    ~MetalDevice();
    
    id<MTLBuffer> GetCbvSrvUavHeap() const;
    
    id<MTLCommandQueue> GetCommandPool() const;
	
	id<MTLResidencySet> GetResidencySet() const;
	
	id<MTLCommandBuffer> AllocateCommandBuffer() const;
	
	IRRootSignature* GetGlobalRootSignature() const;
	
	ShaderResourceIndex CreateResourceView(const Buffer& buffer);

	ShaderResourceIndex CreateResourceView(const Texture& texture);

	void ReleaseResourceView(ShaderResourceIndex view);
    
private:

	virtual void Configure(const RendererConfig& config) override;
    
    MetalDescriptorHeap CreateDescriptorHeap(uint32_t capacity) const;

	IRRootSignature* mRootSignature = nullptr;

    id<MTLCommandQueue> mCommandPool{ nil };
	
	id<MTLResidencySet> mResidencySet{ nil };
    
    MetalDescriptorHeap mCbvSrvUavHeap;

};

} // namespace Gleam
#endif
