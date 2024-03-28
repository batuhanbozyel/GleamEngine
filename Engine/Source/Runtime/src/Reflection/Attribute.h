#pragma once
#include "Core/GUID.h"

#include <refl.hpp>
#include <entt/core/hashed_string.hpp>

namespace Gleam::Reflection {

template<typename T>
concept AttributeType = std::is_base_of_v<refl::attr::usage::type, T>
                     || std::is_base_of_v<refl::attr::usage::field, T>;

struct AttributeDescription
{
    const char* tag;
    uint32_t hash;
    
    explicit constexpr AttributeDescription(const char* str)
        : tag(str)
        , hash(entt::hashed_string(str))
    {
    }
};

namespace Attribute {

namespace Target {
using Class = refl::attr::usage::type;
using Field = refl::attr::usage::field;
} // namespace Type

#define GLEAM_ATTRIBUTE(tag, ...) \
struct AttributeBase_##tag { static constexpr auto description = AttributeDescription(#tag); }; \
struct tag : AttributeBase_##tag, __VA_ARGS__

GLEAM_ATTRIBUTE(Guid, Target::Class)
{
    union
    {
        struct
        {
            uint32_t data1;
            uint16_t data2;
            uint16_t data3;
            uint8_t data4[8];
        };
        TArray<uint8_t, 16> bytes = { 0 };
    };
    
    explicit constexpr Guid(const TStringView str)
    {
        if (str.length() < 20) // 16 bytes + 4 separators
        {
            GLEAM_ASSERT(false, "Invalid Guid string!");
            return;
        }

        auto it = str.data();
        if (*it == '{')
        {
            it++;
        }
        
        data1 = ParseHexDigits<uint32_t>(it);
        data2 = ParseHexDigits<uint16_t>(it);
        data3 = ParseHexDigits<uint16_t>(it);

        data4[0] = ParseHexDigits<uint8_t>(it);
        data4[1] = ParseHexDigits<uint8_t>(it);

        data4[2] = ParseHexDigits<uint8_t>(it);
        data4[3] = ParseHexDigits<uint8_t>(it);
        data4[4] = ParseHexDigits<uint8_t>(it);
        data4[5] = ParseHexDigits<uint8_t>(it);
        data4[6] = ParseHexDigits<uint8_t>(it);
        data4[7] = ParseHexDigits<uint8_t>(it);
    }
};

GLEAM_ATTRIBUTE(Version, Target::Class)
{
    uint32_t version;
    
    explicit constexpr Version(uint32_t version)
        : version(version)
    {
        
    }
};

GLEAM_ATTRIBUTE(Serializable, Target::Field)
{
};

GLEAM_ATTRIBUTE(PrettyName, Target::Class, Target::Field)
{
    TStringView name;
    
    explicit constexpr PrettyName(const TStringView name)
        : name(name)
    {
        
    }
};


} // namespace Attribute
} // namespace Gleam::Reflection
