#pragma once

namespace Gleam {

class RenderSurface
{
public:

	virtual ~RenderSurface() = default;

	virtual void Resize(GraphicsDevice* device, const Size& size) = 0;

	virtual void Present(const CommandBuffer* cmd) = 0;

	virtual TextureFormat GetFormat() const = 0;

	virtual const Size& GetSize() const = 0;

};

} // namespace Gleam
