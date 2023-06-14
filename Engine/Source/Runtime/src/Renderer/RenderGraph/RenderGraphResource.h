#pragma once

namespace Gleam {

struct RenderGraphResource
{
public:
    
    static const RenderGraphResource nullHandle;
    
    explicit constexpr RenderGraphResource(uint32_t uniqueId, uint32_t version)
        : uniqueId(uniqueId), version(version)
    {
        
    }
    
    FORCE_INLINE constexpr operator uint32_t() const
    {
        return uniqueId;
    }
    
    NO_DISCARD FORCE_INLINE constexpr uint32_t GetVersion() const
    {
        return version;
    }
    
    constexpr RenderGraphResource(const RenderGraphResource& other) = default;
    FORCE_INLINE constexpr RenderGraphResource& operator=(const RenderGraphResource& other) = default;
    
private:
    
    uint32_t version;
    uint32_t uniqueId;
    
};

#define RenderGraphResourceHandle(HandleType) class HandleType final : public RenderGraphResource {\
    public: explicit HandleType(uint32_t uniqueId = nullHandle, uint32_t version = 0) : RenderGraphResource(uniqueId, version) {}\
    constexpr HandleType(const RenderGraphResource& other) : RenderGraphResource(other) {}\
    FORCE_INLINE constexpr HandleType& operator=(const RenderGraphResource& other) { RenderGraphResource::operator=(other); return *this; }\
}

RenderGraphResourceHandle(BufferHandle);
RenderGraphResourceHandle(RenderTextureHandle);

} // namespace Gleam
