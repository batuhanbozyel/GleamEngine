#include "gpch.h"
#include "Texture2D.h"

#include "Core/Engine.h"
#include "Core/Globals.h"
#include "Renderer/RenderSystem.h"

using namespace Gleam;

Texture2D::Texture2D(const Texture2DDescriptor& descriptor)
{
	GLEAM_ASSERT(descriptor.dimension == TextureDimension::Texture2D, "Texture2D descriptor dimension mismatch.");

	static auto renderSystem = Globals::Engine->GetSubsystem<RenderSystem>();
	mTexture = renderSystem->GetDevice()->CreateTexture(descriptor);

	// Send texture data to buffers
	if (descriptor.pixels.empty() == false)
	{
		// TODO: use upload manager to send data to the GPU
	}
}

void Texture2D::Release()
{
	static auto renderSystem = Globals::Engine->GetSubsystem<RenderSystem>();
	auto device = renderSystem->GetDevice();
	device->ReleaseTexture(mTexture);
}

ShaderResourceIndex Texture2D::GetResourceView() const
{
	return mTexture.GetResourceView();
}