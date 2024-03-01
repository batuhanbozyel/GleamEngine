#pragma once

namespace Gleam {

class Buffer;
class Texture;
class RenderGraph;
class RenderGraphBuilder;
struct RenderGraphBufferNode;
struct RenderGraphTextureNode;

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

    explicit constexpr ResourceHandle(uint32_t version, ResourceAccess access)
        : version(version), access(access)
    {
        
    }
    
    NO_DISCARD FORCE_INLINE constexpr ResourceAccess GetAccess() const
    {
        return access;
    }
    
    constexpr ResourceHandle(const ResourceHandle&) = default;
    FORCE_INLINE constexpr ResourceHandle& operator=(const ResourceHandle&) = default;
    
protected:
    
    uint32_t version;
    ResourceAccess access;
    
};

class BufferHandle final : public ResourceHandle
{
    friend class RenderGraph;
    friend class RenderGraphBuilder;
    
public:
    
    explicit BufferHandle(RenderGraphBufferNode* node = nullptr, uint32_t version = 0, ResourceAccess access = ResourceAccess::None)
        : ResourceHandle(version, access), node(node)
    {
        
    }
    
    constexpr BufferHandle(const BufferHandle& other)
        : ResourceHandle(other), node(other.node)
    {
        
    }
    
    FORCE_INLINE constexpr BufferHandle& operator=(const BufferHandle& other)
    {
        ResourceHandle::operator=(other);
        node = other.node;
        return *this;
    }
    
    FORCE_INLINE constexpr bool operator==(const BufferHandle& other) const
    {
        return node == other.node;
    }
    
    FORCE_INLINE constexpr bool operator!=(const BufferHandle& other) const
    {
        return !(*this == other);
    }
    
    NO_DISCARD FORCE_INLINE constexpr bool IsValid() const
    {
        return node != nullptr;
    }

	NO_DISCARD operator Buffer() const;
    
    NO_DISCARD operator ShaderResourceIndex() const;
    
private:
    
    RenderGraphBufferNode* node;
    
};

class TextureHandle final : public ResourceHandle
{
    friend class RenderGraph;
    friend class RenderGraphBuilder;
    
public:
    
    explicit TextureHandle(RenderGraphTextureNode* node = nullptr, uint32_t version = 0, ResourceAccess access = ResourceAccess::None)
        : ResourceHandle(version, access), node(node)
    {
        
    }
    
    constexpr TextureHandle(const TextureHandle& other)
        : ResourceHandle(other), node(other.node)
    {
        
    }
    
    FORCE_INLINE constexpr TextureHandle& operator=(const TextureHandle& other)
    {
        ResourceHandle::operator=(other);
        node = other.node;
        return *this;
    }
    
    FORCE_INLINE constexpr bool operator==(const TextureHandle& other) const
    {
        return node == other.node;
    }
    
    FORCE_INLINE constexpr bool operator!=(const TextureHandle& other) const
    {
        return !(*this == other);
    }
    
    NO_DISCARD FORCE_INLINE constexpr bool IsValid() const
    {
        return node != nullptr;
    }

    NO_DISCARD operator Texture() const;
    
private:
    
    RenderGraphTextureNode* node;
    
};

} // namespace Gleam
