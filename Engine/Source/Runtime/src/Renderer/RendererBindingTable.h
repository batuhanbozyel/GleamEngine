#pragma once

namespace Gleam {

namespace RendererBindingTable {
enum
{
#if defined(__METAL_VERSION__) || defined(__OBJC__)
    PushConstantBlock,
#endif
    Buffer0,
    Buffer1,
    Buffer2,
    Buffer3,
    Buffer4,
	Buffer5,
    Buffer6,
    
    CameraBuffer = Buffer0,
    PositionBuffer = Buffer1,
    InterleavedBuffer = Buffer2
};
} // namespace RendererBindingTable
    
} // namespace Gleam
