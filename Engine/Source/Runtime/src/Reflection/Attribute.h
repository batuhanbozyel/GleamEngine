#pragma once
#include "Macro.h"
#include "Core/GUID.h"

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
        , hash(StringUtils::Hash(str))
    {
    }
};

namespace Attribute {

namespace Target {
using Class = refl::attr::usage::type;
using Field = refl::attr::usage::field;
} // namespace Type

GLEAM_ATTRIBUTE(Guid, Target::Class)
{
	TArray<uint8_t, 16> bytes;

	template<size_t N>
    explicit constexpr Guid(const char (&str)[N])
		: bytes{
			static_cast<uint8_t>((HexDigitToByte(str[6]) << 4) | (HexDigitToByte(str[7]) << 0)),
			static_cast<uint8_t>((HexDigitToByte(str[4]) << 4) | (HexDigitToByte(str[5]) << 0)),
			static_cast<uint8_t>((HexDigitToByte(str[2]) << 4) | (HexDigitToByte(str[3]) << 0)),
			static_cast<uint8_t>((HexDigitToByte(str[0]) << 4) | (HexDigitToByte(str[1]) << 0)),

			// str[8] = separator

			static_cast<uint8_t>((HexDigitToByte(str[11]) << 4) | (HexDigitToByte(str[12]) << 0)),
			static_cast<uint8_t>((HexDigitToByte(str[9]) << 4) | (HexDigitToByte(str[10]) << 0)),

			// str[13] = separator

			static_cast<uint8_t>((HexDigitToByte(str[16]) << 4) | (HexDigitToByte(str[17]) << 0)),
			static_cast<uint8_t>((HexDigitToByte(str[14]) << 4) | (HexDigitToByte(str[15]) << 0)),

			// str[18] = separator

			static_cast<uint8_t>((HexDigitToByte(str[19]) << 4) | (HexDigitToByte(str[20]) << 0)),
			static_cast<uint8_t>((HexDigitToByte(str[21]) << 4) | (HexDigitToByte(str[22]) << 0)),

			// str[23] = separator
			static_cast<uint8_t>((HexDigitToByte(str[24]) << 4) | (HexDigitToByte(str[25]) << 0)),
			static_cast<uint8_t>((HexDigitToByte(str[26]) << 4) | (HexDigitToByte(str[27]) << 0)),
			static_cast<uint8_t>((HexDigitToByte(str[28]) << 4) | (HexDigitToByte(str[29]) << 0)),
			static_cast<uint8_t>((HexDigitToByte(str[30]) << 4) | (HexDigitToByte(str[31]) << 0)),
			static_cast<uint8_t>((HexDigitToByte(str[32]) << 4) | (HexDigitToByte(str[33]) << 0)),
			static_cast<uint8_t>((HexDigitToByte(str[34]) << 4) | (HexDigitToByte(str[35]) << 0))
		}
    {
		static_assert(N == 37); // 32 bytes + 4 separators + null terminator
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

GLEAM_ATTRIBUTE(EntityComponent, Target::Class)
{
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
