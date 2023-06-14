#pragma once
#include "Texture.h"

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
    RefCounted<RenderTexture> texture;
	AttachmentLoadAction loadAction = AttachmentLoadAction::Load;
	AttachmentStoreAction storeAction = AttachmentStoreAction::Store;
    
    bool operator==(const AttachmentDescriptor& other) const
    {
        return  clearColor == other.clearColor &&
                (Math::Abs(clearDepth - other.clearDepth) < Math::Epsilon) &&
                clearStencil == other.clearStencil &&
                texture->GetDescriptor() == other.texture->GetDescriptor() &&
                loadAction == other.loadAction &&
                storeAction == other.storeAction;
    }
};

struct RenderPassDescriptor
{
	TArray<AttachmentDescriptor> colorAttachments;
    AttachmentDescriptor depthAttachment;
    Size size = Size::zero;
	uint32_t samples = 1;
	bool useMipMap = false;
    
    bool operator==(const RenderPassDescriptor& other) const
    {
        if (colorAttachments.size() != other.colorAttachments.size()) return false;
        for (uint32_t i = 0; i < colorAttachments.size(); i++)
        {
            if (colorAttachments[i] != other.colorAttachments[i]) { return false; }
        }
        return depthAttachment == other.depthAttachment && size == other.size && samples == other.samples && useMipMap == other.useMipMap;
    }
};

} // namespace Gleam
