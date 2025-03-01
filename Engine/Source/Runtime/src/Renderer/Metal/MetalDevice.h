#pragma once
#ifdef USE_METAL_RENDERER
#include "Renderer/GraphicsDevice.h"

#include <Metal/Metal.h>
#include <metal_irconverter/metal_irconverter.h>

namespace Gleam {

struct Version;
struct RendererConfig;

struct MetalPipeline
{
	virtual ~MetalPipeline() = default;
};

struct MetalGraphicsPipeline : public MetalPipeline
{
	id<MTLRenderPipelineState> renderState = nil;
	id<MTLDepthStencilState> depthStencilState = nil;
	MTLPrimitiveType topology = MTLPrimitiveTypeTriangle;
};

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
    
    virtual ShaderResourceIndex CreateResourceView(const Buffer& buffer) override;
    
    virtual ShaderResourceIndex CreateResourceView(const Texture& texture) override;
    
    virtual void ReleaseResourceView(ShaderResourceIndex view) override;
    
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
