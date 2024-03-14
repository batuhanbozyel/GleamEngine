#pragma once
#include "../GraphicsDevice.h"
#include <imgui.h>

namespace Gleam {

class ImGuiBackend
{
public:

	static void Init(GraphicsDevice* device);

	static void Destroy();

	static void BeginFrame();

	static void EndFrame(NativeGraphicsHandle commandBuffer, NativeGraphicsHandle renderCommandEncoder);

	static ImTextureID GetImTextureIDForTexture(const Texture& texture);
    
private:
    
    static inline GraphicsDevice* mDevice = nullptr;

};

} // namespace Gleam
