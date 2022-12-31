#pragma once
#include "TextureFormat.h"

namespace Gleam {

enum class AttachmentLoadAction
{
	Load,
	Clear,
	DontCare
};

enum class AttachmentStoreAction
{
	Store,
	Resolve,
	StoreAndResolve,
	DontCare
};

struct AttachmentDescriptor
{
	Color clearColor = Color::clear;
	float clearDepth = 1.0f;
	uint32_t clearStencil = 0;
	TextureFormat format = TextureFormat::None;
	AttachmentLoadAction loadAction = AttachmentLoadAction::Load;
	AttachmentStoreAction storeAction = AttachmentStoreAction::Store;
    
    constexpr bool operator==(const AttachmentDescriptor&) const = default;
};

struct RenderPassDescriptor
{
	TArray<AttachmentDescriptor> attachments;
    Size size = Size::zero;
	int depthAttachmentIndex = -1;
	uint32_t samples = 1;
	bool useMipMap = false;
    
    bool operator==(const RenderPassDescriptor&) const = default;
};

} // namespace Gleam
