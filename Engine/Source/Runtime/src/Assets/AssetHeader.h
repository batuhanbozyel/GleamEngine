#pragma once
#include "Core/GUID.h"
#include "Core/Macro.h"
#include "Container/Array.h"
#include "Container/BinaryBuffer.h"

namespace Gleam {

GENUM(AssetPlatform, "44595E67-4DF5-4CC2-A855-E618A7C6FD95", Serializable)
{
	GITEM(Common, "B8B9F006-5748-46F8-9762-E08D1AD2CB77"),
	GITEM(Windows, "5455586B-6990-43A4-AA7F-91373850CDBE"),
	GITEM(MacOS, "3D5C162E-2CE7-427C-BA96-E39EE0C817EE"),
	GITEM(IOS, "47C7605D-DBF8-4268-9E10-ADA2857E1497"),
	GITEM(Android, "117ED490-81DF-406F-9F5A-15209F1EC1CA"),
	GITEM(Linux, "708BF6C3-6075-4EC2-AB4D-4CDCCC70763A")
};

GENUM(AssetBackend, "5DB6AC17-7C19-46F5-8FCB-73823F81A1D6", Serializable)
{
	GITEM(Common, "DEAB3C73-4459-482C-A816-EF221CE82F97"),
	GITEM(DirectX, "5700379E-4363-4755-89AB-0B354F59D062"),
	GITEM(Metal, "7F1C2BB5-41DF-4B90-BC85-8C74CBF23EB0")
};

GSTRUCT(AssetBlobDescriptor, "F6A13231-A72D-4E98-9A04-86F068EBECB7", Serializable, Version(1))
{
	GFIELD("930F28E3-A863-4BBE-B2A6-44E59BA7CA76", Serializable)
	uint32_t slot = 0;

	GFIELD("73A3DB9F-7198-4089-8E84-6F72D4DB0C8F", Serializable)
	AssetPlatform platform = AssetPlatform::Common;

	GFIELD("A005297D-78A2-40C5-BE40-521B138DEE74", Serializable)
	AssetBackend backend = AssetBackend::Common;

	GFIELD("836E0300-2FDC-4E99-9B91-47C6A3BC2B28", Serializable)
	uint64_t layoutHash = 0;

	GFIELD("E56AC771-131C-4189-B9CB-C7819F33F73C", Serializable)
	BufferRange range;
};

GSTRUCT(AssetDataTable, "91170B46-96F7-48CE-9B16-9946705CF8C6", Serializable, Version(1))
{
	GFIELD("24FD8C88-4A57-4BAA-9BAE-0EE8663FEF5D", Serializable)
	TArray<AssetBlobDescriptor> blobs;
};

GSTRUCT(AssetHeader, "ADBF5512-90F9-4F59-B4B4-E2834DA8C731", Serializable, Version(1))
{
	GFIELD("7AE4E1DC-520F-455F-8773-0F0F708A36D9", Serializable)
	Guid typeGuid;

	GFIELD("C89824CA-F29B-4136-8E80-68F77E48CAB3", Serializable)
	uint32_t blobCount = 0;

	GFIELD("66B7D442-3FC1-4DFB-967F-E0E06B847BEF", Serializable)
	BufferRange name;

	GFIELD("78770ABA-6737-441A-86EA-AAD59E63E38D", Serializable)
	BufferRange dataTable;

	GFIELD("6E03A27B-6E62-4ED5-9986-8AC1DCE7CB96", Serializable)
	BufferRange metadata;

	GFIELD("09C8BCBF-F3D9-4C68-B5CE-135FAEB5AF5A", Serializable)
	BufferRange bulkData;
};

namespace AssetUtils {

NO_DISCARD constexpr AssetPlatform Platform()
{
#if defined(PLATFORM_WINDOWS)
	return AssetPlatform::Windows;
#elif defined(PLATFORM_MACOS)
	return AssetPlatform::MacOS;
#elif defined(PLATFORM_IOS)
	return AssetPlatform::IOS;
#elif defined(PLATFORM_ANDROID)
	return AssetPlatform::Android;
#else
	return AssetPlatform::Linux;
#endif
}

NO_DISCARD constexpr AssetBackend RenderBackend()
{
#ifdef USE_METAL_RENDERER
	return AssetBackend::Metal;
#else
	return AssetBackend::DirectX;
#endif
}

} // namespace AssetUtils

} // namespace Gleam
