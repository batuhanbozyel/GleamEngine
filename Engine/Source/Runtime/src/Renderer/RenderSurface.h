#pragma once

namespace Gleam {

struct Size;
enum class TextureFormat;

class GraphicsDevice;
class CommandBuffer;

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
