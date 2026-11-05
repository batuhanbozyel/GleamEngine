#include "gpch.h"
#include "RenderTexture.h"
#include "RenderSystem.h"
#include "GraphicsDevice.h"

#include "Core/Engine.h"
#include "Core/Globals.h"

using namespace Gleam;

RenderTexture::RenderTexture(const AssetReference& reference, const AssetHeader& header, const RenderTextureDescriptor& descriptor)
	: Asset(reference, header)
	, mClearColor(descriptor.clearColor)
	, mClearBuffer(descriptor.clearBuffer)
{
	static auto renderSystem = Globals::Engine->GetSubsystem<RenderSystem>();

	TextureDescriptor textureDescriptor = descriptor;
	textureDescriptor.name = "RenderTexture: " + (descriptor.name.empty() ? header.name : descriptor.name);
	mTexture = renderSystem->GetDevice()->CreateTexture(renderSystem->GetAllocator(), textureDescriptor);
}

RenderTexture::~RenderTexture()
{
	static auto renderSystem = Globals::Engine->GetSubsystem<RenderSystem>();
	renderSystem->GetDevice()->Dispose(renderSystem->GetAllocator(), mTexture, BarrierStage::None);
}

void RenderTexture::Resize(GraphicsDevice* device, const Size& size)
{
	if (mTexture.GetDescriptor().size == size)
	{
		return;
	}

	static auto renderSystem = Globals::Engine->GetSubsystem<RenderSystem>();
	device->Dispose(renderSystem->GetAllocator(), mTexture, BarrierStage::None);

	TextureDescriptor descriptor = mTexture.GetDescriptor();
	descriptor.size = size;
	mTexture = device->CreateTexture(renderSystem->GetAllocator(), descriptor);
}
