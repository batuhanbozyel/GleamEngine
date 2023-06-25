#pragma once
#ifdef USE_METAL_RENDERER
#include "MetalSwapchain.h"

namespace Gleam {

struct Version;
struct RendererConfig;

class MetalDevice final
{
public:

	static void Init();

    static void Destroy();

	static MetalSwapchain& GetSwapchain();

	static id<MTLCommandQueue> GetCommandPool();

	static id<MTLLibrary> GetShaderLibrary();

	static id<MTLDevice> GetHandle();

private:

	static inline MetalSwapchain mSwapchain;

	static inline id<MTLCommandQueue> mCommandPool{ nil };

	static inline id<MTLLibrary> mShaderLibrary{ nil };

	static inline id<MTLDevice> mHandle{ nil };

};

} // namespace Gleam
#endif
