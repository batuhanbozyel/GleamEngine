#include "gpch.h"
#include "Texture2D.h"

#include "Core/Engine.h"
#include "Core/Globals.h"

#include "IO/File.h"
#include "IO/Filesystem.h"

#include "Assets/AssetManager.h"

#include "Container/BinaryBuffer.h"

#include "Renderer/RenderSystem.h"
#include "Renderer/GraphicsDevice.h"
#include "Renderer/CopyCommandBuffer.h"

using namespace Gleam;

Texture2D::Texture2D(const AssetReference& reference, const AssetHeader& header, const Texture2DDescriptor& descriptor)
	: Asset(reference, header)
{
	GLEAM_ASSERT(descriptor.dimension == TextureDimension::Texture2D, "Texture2D descriptor dimension mismatch.");

	static auto renderSystem = Globals::Engine->GetSubsystem<RenderSystem>();
	static auto assetManager = Globals::GameInstance->GetSubsystem<AssetManager>();

	mTexture = renderSystem->GetDevice()->CreateTexture(renderSystem->GetAllocator(), descriptor);

	auto storage = assetManager->GetStorage();
	auto cmd = renderSystem->GetCopyCommandBuffer();

	auto file = Filesystem::OpenRead(storage->GetAssetFilePath(GetReference()), FileType::Binary);
	auto& stream = file->GetStream();

	BinaryBuffer blobData;
	for (uint32_t i = 0; i < descriptor.subresources.size(); ++i)
	{
		const auto blob = FindBlob<TextureSubresourceDescriptor>(descriptor.subresources[i].blobSlot, AssetPlatform::Common, AssetBackend::Common);
		if (blob == nullptr)
		{
			continue;
		}

		const auto range = GetBlobRange(*blob);
		blobData.Resize(range.size);

		stream.seekg(static_cast<std::streamoff>(range.offset));
		stream.read(static_cast<char*>(blobData.data), static_cast<std::streamsize>(range.size));

		cmd->Commit(mTexture, blobData.data, blobData.size, mTexture.GetMip(i), mTexture.GetSlice(i));
	}
}

Texture2D::Texture2D(const TextureDescriptor& descriptor, const void* pixels, size_t size)
	: Asset(AssetReference(), AssetHeader{ .name = descriptor.name })
{
	GLEAM_ASSERT(descriptor.dimension == TextureDimension::Texture2D, "Texture2D descriptor dimension mismatch.");

	static auto renderSystem = Globals::Engine->GetSubsystem<RenderSystem>();
	mTexture = renderSystem->GetDevice()->CreateTexture(renderSystem->GetAllocator(), descriptor);

	auto cmd = renderSystem->GetCopyCommandBuffer();
	cmd->Commit(mTexture, pixels, size, 0, 0);
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
