#pragma once

namespace Gleam {

class CommandBuffer
{
public:
    
    void ClearRenderTarget(bool clearColor, bool clearDepth, const Color& backgroundColor = Color::Clear, float depth = 1.0f);
    
private:
    
    
};

} // namespace Gleam
