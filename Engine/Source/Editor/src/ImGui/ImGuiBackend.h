#pragma once
#include "Gleam.h"
#include <imgui.h>

namespace GEditor {

class ImGuiBackend
{
public:

	static void Init();

	static void Destroy();

	static void BeginFrame();

	static void EndFrame(NativeGraphicsHandle commandBuffer, NativeGraphicsHandle renderCommandEncoder);

	static ImTextureID GetImTextureIDForTexture(const Gleam::Texture& texture);

};

} // namespace GEditor
