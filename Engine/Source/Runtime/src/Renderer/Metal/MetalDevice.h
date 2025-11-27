#pragma once
#ifdef USE_METAL_RENDERER
#include "Renderer/GraphicsDevice.h"

#include <Metal/Metal.h>
#include <metal_irconverter/metal_irconverter.h>

@protocol MetalPipeline <NSObject>
@end

@protocol MetalGraphicsPipeline <MetalPipeline>

@property (nonatomic, strong) id<MTLRenderPipelineState> renderState;
@property (nonatomic, strong) id<MTLDepthStencilState> depthStencilState;
@property (nonatomic, assign) MTLPrimitiveType topology;

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
    
private:

	virtual void Configure(const RendererConfig& config) override;

	ShaderResourceIndex CreateResourceView(const Buffer& buffer);

	ShaderResourceIndex CreateResourceView(const Texture& texture);

	void ReleaseResourceView(ShaderResourceIndex view);
    
    MetalDescriptorHeap CreateDescriptorHeap(uint32_t capacity) const;

	IRRootSignature* mRootSignature = nullptr;

    id<MTLCommandQueue> mCommandPool{ nil };
	
	id<MTLResidencySet> mResidencySet{ nil };
    
    MetalDescriptorHeap mCbvSrvUavHeap;

};

} // namespace Gleam
#endif
