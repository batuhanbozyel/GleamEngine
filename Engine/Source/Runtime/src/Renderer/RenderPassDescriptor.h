#pragma once
#include "TextureFormat.h"

namespace Gleam {

class RenderTexture;

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
    RefCounted<RenderTexture> texture = nullptr;
	AttachmentLoadAction loadAction = AttachmentLoadAction::Load;
	AttachmentStoreAction storeAction = AttachmentStoreAction::Store;
    
    bool operator==(const AttachmentDescriptor&) const = default;
};

struct RenderPassDescriptor
{
	TArray<AttachmentDescriptor> colorAttachments;
    AttachmentDescriptor depthAttachment;
    Size size = Size::zero;
	uint32_t samples = 1;
	bool useMipMap = false;
    
    bool operator==(const RenderPassDescriptor&) const = default;
};

} // namespace Gleam
