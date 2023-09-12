#pragma once

namespace Gleam {

class Buffer;
class Texture;
class RenderGraph;
class RenderGraphBuilder;
struct RenderGraphResourceNode;

enum class ResourceAccess
{
	None,
    Read,
    Write
};

class ResourceHandle
{
	friend class RenderGraph;
	friend class RenderGraphBuilder;

public:

    explicit constexpr ResourceHandle(RenderGraphResourceNode* node = nullptr, uint32_t version = 0, ResourceAccess access = ResourceAccess::None)
        : node(node), version(version), access(access)
    {
        
    }

	NO_DISCARD FORCE_INLINE constexpr bool IsValid() const
    {
        return node != nullptr;
    }
    
    NO_DISCARD FORCE_INLINE constexpr ResourceAccess GetAccess() const
    {
        return access;
    }
    
    constexpr ResourceHandle(const ResourceHandle&) = default;
    FORCE_INLINE constexpr ResourceHandle& operator=(const ResourceHandle&) = default;
	FORCE_INLINE constexpr bool operator==(const ResourceHandle&) const = default;
	FORCE_INLINE constexpr bool operator!=(const ResourceHandle&) const = default;
    
protected:
    
    uint32_t version;
    ResourceAccess access;
    RenderGraphResourceNode* node;
    
};

#define RenderGraphResourceHandle(HandleType) explicit HandleType(RenderGraphResourceNode* node = nullptr, uint32_t version = 0, ResourceAccess access = ResourceAccess::None) : ResourceHandle(node, version, access) {}\
    constexpr HandleType(const ResourceHandle& other) : ResourceHandle(other) {}\
    FORCE_INLINE constexpr HandleType& operator=(const ResourceHandle& other) { ResourceHandle::operator=(other); return *this; }\

class BufferHandle final : public ResourceHandle
{
public:

	RenderGraphResourceHandle(BufferHandle)

	NO_DISCARD operator Buffer() const;
};

class TextureHandle final : public ResourceHandle
{
public:

	RenderGraphResourceHandle(TextureHandle)

    NO_DISCARD operator Texture() const;
};

} // namespace Gleam
