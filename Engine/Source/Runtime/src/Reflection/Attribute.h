#pragma once
#include "Core/GUID.h"

#include <refl.hpp>

namespace Gleam::Reflection {

namespace Attribute {

struct Guid : refl::attr::usage::type
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

struct Version : refl::attr::usage::type
{
    uint32_t version;
    
    explicit constexpr Version(uint32_t version)
        : version(version)
    {
        
    }
};

struct Serializable : refl::attr::usage::field
{
    
};

struct PrettyName : refl::attr::usage::type,
                    refl::attr::usage::field
{
    TStringView name;
};

} // namespace Reflection::Attribute

} // namespace Gleam
