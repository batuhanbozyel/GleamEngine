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
    
    bool operator==(const AttachmentDescriptor&) const = default;
};

struct SubpassDescriptor
{
	TArray<uint32_t> colorAttachments = { 0 };
	TArray<uint32_t> inputAttachments = { 0 };
    
    bool operator==(const SubpassDescriptor&) const = default;
};

struct RenderPassDescriptor
{
	TArray<AttachmentDescriptor> attachments = {};
	TArray<SubpassDescriptor> subpasses = {};
	uint32_t width = 0, height = 0;
	int depthAttachmentIndex = -1;
	uint32_t samples = 1;
	uint32_t mipLevels = 0;
	bool swapchainTarget = false;
    
    bool operator==(const RenderPassDescriptor&) const = default;
};

} // namespace Gleam
