#include "gpch.h"
#include "RenderTexture.h"
#include "RenderSystem.h"
#include "GraphicsDevice.h"

#include "Core/Engine.h"
#include "Core/Globals.h"

using namespace Gleam;

RenderTexture::RenderTexture(const AssetReference& reference, const AssetHeader& header, const RenderTextureDescriptor& descriptor)
	: Asset(reference, header)
	, mDescriptor(descriptor)
{
	if (mDescriptor.name.empty())
	{
		mDescriptor.name = header.name;
	}
	CreateTexture();
}

RenderTexture::~RenderTexture()
{
	static auto renderSystem = Globals::Engine->GetSubsystem<RenderSystem>();
	renderSystem->GetDevice()->Dispose(renderSystem->GetAllocator(), mTexture, BarrierStage::None);
}

void RenderTexture::Resize(const Size& size)
{
	if (mDescriptor.size == size)
	{
		return;
	}

	static auto renderSystem = Globals::Engine->GetSubsystem<RenderSystem>();
	renderSystem->GetDevice()->Dispose(renderSystem->GetAllocator(), mTexture, BarrierStage::None);
	mDescriptor.size = size;
	CreateTexture();
}

void RenderTexture::CreateTexture()
{
	static auto renderSystem = Globals::Engine->GetSubsystem<RenderSystem>();
	mTexture = renderSystem->GetDevice()->CreateTexture(renderSystem->GetAllocator(), mDescriptor);
}
