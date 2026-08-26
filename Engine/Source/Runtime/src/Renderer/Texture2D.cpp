#include "gpch.h"
#include "Texture2D.h"

#include "Core/Engine.h"
#include "Core/Globals.h"

#include "Renderer/RenderSystem.h"
#include "Renderer/GraphicsDevice.h"
#include "Renderer/CopyCommandBuffer.h"

using namespace Gleam;

Texture2D::Texture2D(const AssetReference& reference, const AssetHeader& header, const Texture2DDescriptor& descriptor)
	: Asset(reference, header)
{
	GLEAM_ASSERT(descriptor.dimension == TextureDimension::Texture2D, "Texture2D descriptor dimension mismatch.");

	static auto renderSystem = Globals::Engine->GetSubsystem<RenderSystem>();
	mTexture = renderSystem->GetDevice()->CreateTexture(renderSystem->GetAllocator(), descriptor);

	// Send texture data to buffers
	auto cmd = renderSystem->GetCopyCommandBuffer();
	for (uint32_t i = 0; i < descriptor.subresources.size(); ++i)
	{
		const auto& subresource = descriptor.subresources[i];
		cmd->Commit(mTexture, OffsetPointer(descriptor.pixels.data, subresource.offset), subresource.size, mTexture.GetMip(i), mTexture.GetSlice(i));
	}
}

Texture2D::~Texture2D()
{
	static auto renderSystem = Globals::Engine->GetSubsystem<RenderSystem>();
	auto device = renderSystem->GetDevice();
	device->Dispose(renderSystem->GetAllocator(), mTexture, BarrierStage::None);
}

ShaderResourceIndex Texture2D::GetResourceView() const
{
	return mTexture.GetResourceView();
}
