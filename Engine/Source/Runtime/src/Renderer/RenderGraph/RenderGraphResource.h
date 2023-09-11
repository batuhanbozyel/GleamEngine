#pragma once

namespace Gleam {

class Buffer;
class Texture;
struct RenderGraphResourceNode;

enum class ResourceAccess
{
	None,
    Read,
    Write
};

class ResourceHandle
{
public:
    static const ResourceHandle nullHandle;
    
    explicit constexpr ResourceHandle(RenderGraphResourceNode* node = nullptr, uint32_t version = 0, ResourceAccess access = ResourceAccess::None)
        : node(node), version(version), access(access)
    {
        
    }

	NO_DISCARD FORCE_INLINE constexpr bool IsValid() const
    {
        return node != nullptr;
    }
    
    NO_DISCARD FORCE_INLINE constexpr uint32_t GetVersion() const
    {
        return version;
    }
    
    NO_DISCARD FORCE_INLINE constexpr ResourceAccess GetAccess() const
    {
        return access;
    }
    
    constexpr ResourceHandle(const ResourceHandle& other) = default;
    FORCE_INLINE constexpr ResourceHandle& operator=(const ResourceHandle& other) = default;
    
private:
    
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

	NO_DISCARD constexpr operator Buffer() const;
};

class TextureHandle final : public ResourceHandle
{
public:

	RenderGraphResourceHandle(TextureHandle)

    NO_DISCARD FORCE_INLINE constexpr operator Texture() const;
};

} // namespace Gleam
