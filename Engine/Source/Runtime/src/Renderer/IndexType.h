#pragma once
#include <concepts>

namespace Gleam {
    
enum class IndexType
{
    UINT16,
    UINT32
};

template<typename T>
concept IndexBufferTypes = std::same_as<T, uint32_t> || std::same_as<T, uint16_t>;

template<IndexBufferTypes T>
struct IndexTraits;

template<>
struct IndexTraits<uint16_t> { static constexpr IndexType indexType = IndexType::UINT16; };

template<>
struct IndexTraits<uint32_t> { static constexpr IndexType indexType = IndexType::UINT32; };
    
static constexpr size_t SizeOfIndexType(IndexType indexType)
{
    switch (indexType)
    {
        case IndexType::UINT16: return sizeof(uint16_t);
        case IndexType::UINT32: return sizeof(uint32_t);
        default: return 0;
    }
}
    
} // namespace Gleam
