#pragma once
#ifdef USE_METAL_RENDERER
#include "Renderer/GraphicsDevice.h"
#include <Metal/Metal.h>

namespace Gleam {

struct Version;
struct RendererConfig;

class MetalDevice final : public GraphicsDevice
{
public:
    
    MetalDevice();
    
    ~MetalDevice();
    
    id<MTLDevice> GetHandle() const;
    
    id<MTLCommandQueue> GetCommandPool() const;
    
private:
    
    id<MTLCommandQueue> mCommandPool{ nil };

};

} // namespace Gleam
#endif
