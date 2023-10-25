#pragma once
#include "Gleam.h"
#include <imgui.h>

namespace GEditor {

class ImGuiBackend
{
public:

	static void Init(Gleam::GraphicsDevice* device);

	static void Destroy();

	static void BeginFrame();

	static void EndFrame(NativeGraphicsHandle commandBuffer, NativeGraphicsHandle renderCommandEncoder);

	static ImTextureID GetImTextureIDForTexture(const Gleam::Texture& texture);
    
private:
    
    static inline Gleam::GraphicsDevice* mDevice = nullptr;

};

} // namespace GEditor
