#pragma once
#include "../Shaders/ShaderInterop.h"
#include "../Texture.h"
#include "../Buffer.h"

namespace Gleam {

class RenderGraph;
class RenderGraphBuilder;
struct RenderGraphBufferNode;
struct RenderGraphTextureNode;

template <typename T>
concept TextureResourceViewType = std::is_base_of<TextureResourceView, T>::value;

enum class ResourceAccess
{
    Read,
    Write,
    None
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
    
    explicit BufferHandle(RenderGraphBufferNode* node = nullptr,
                          uint32_t version = 0,
                          ResourceAccess access = ResourceAccess::None)
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

    NO_DISCARD operator ConstantBufferView() const
    {
        GLEAM_ASSERT(access == ResourceAccess::Read);
        GLEAM_ASSERT(GetBuffer().GetResourceView() != InvalidResourceIndex);
        ConstantBufferView cbv = GetBuffer().GetResourceView();
        return cbv;
    }
    
    NO_DISCARD operator BufferResourceView() const
    {
        GLEAM_ASSERT(access == ResourceAccess::Read);
        GLEAM_ASSERT(GetBuffer().GetResourceView() != InvalidResourceIndex);
        BufferResourceView srv = GetBuffer().GetResourceView();
        return srv;
    }
    
    NO_DISCARD operator Buffer() const
    {
        return GetBuffer();
    }
    
	NO_DISCARD const Buffer& GetBuffer() const;
    
private:
    
    RenderGraphBufferNode* node;
    
};

class TextureHandle final : public ResourceHandle
{
    friend class RenderGraph;
    friend class RenderGraphBuilder;
    
public:
    
    explicit TextureHandle(RenderGraphTextureNode* node = nullptr,
                           uint32_t version = 0,
                           ResourceAccess access = ResourceAccess::None)
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
    
    template<TextureResourceViewType T>
    NO_DISCARD operator T() const
    {
        GLEAM_ASSERT(access == ResourceAccess::Read);
        GLEAM_ASSERT(GetTexture().GetResourceView() != InvalidResourceIndex);
        T srv = GetTexture().GetResourceView();
        return srv;
    }
    
    NO_DISCARD operator Texture() const
    {
        return GetTexture();
    }

    NO_DISCARD const Texture& GetTexture() const;
    
private:
    
    RenderGraphTextureNode* node;
    
};

} // namespace Gleam
