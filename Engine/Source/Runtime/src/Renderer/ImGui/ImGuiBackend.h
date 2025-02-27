#pragma once
#include <imgui.h>

namespace Gleam {

class RenderContext;
class GraphicsDevice;

class ImGuiBackend
{
public:

	static void Init(RenderContext& context);

	static void Destroy();

	static void BeginFrame();

	static void EndFrame(NativeGraphicsHandle commandBuffer, NativeGraphicsHandle renderCommandEncoder);

	static ImTextureID GetImTextureIDForTexture(const Texture& texture);
    
private:
    
    static inline GraphicsDevice* mDevice = nullptr;

};

} // namespace Gleam
