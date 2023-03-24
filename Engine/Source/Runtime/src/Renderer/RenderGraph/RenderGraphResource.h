#pragma once

namespace Gleam {

class RenderGraphResource
{
public:
    
    static const RenderGraphResource nullHandle;
    
    explicit constexpr RenderGraphResource(uint32_t uniqueId)
        : uniqueId(uniqueId)
    {
        
    }
    
    operator uint32_t() const
    {
        return uniqueId;
    }
    
    constexpr RenderGraphResource(const RenderGraphResource& other) = default;
    FORCE_INLINE constexpr RenderGraphResource& operator=(const RenderGraphResource& other) = default;
    
private:
    
    uint32_t uniqueId;
    
};

#define RenderGraphResourceHandle(HandleType) class HandleType : public RenderGraphResource {\
    public: explicit HandleType(uint32_t uniqueId = nullHandle) : RenderGraphResource(uniqueId) {}\
    constexpr HandleType(const RenderGraphResource& other) : RenderGraphResource(other) {}\
    FORCE_INLINE constexpr HandleType& operator=(const RenderGraphResource& other) { RenderGraphResource::operator=(other); return *this; }\
}

RenderGraphResourceHandle(BufferHandle);
RenderGraphResourceHandle(RenderTextureHandle);

} // namespace Gleam
