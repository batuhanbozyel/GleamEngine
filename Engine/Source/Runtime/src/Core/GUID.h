#pragma once

namespace Gleam {

namespace Reflection::Attribute {
struct Guid;
} // Reflection::Attribute

static constexpr uint8_t HexDigitToByte(char ch)
{
    // 0-9
    if (ch >= '0' && ch <= '9')
        return ch - '0';

    // a-f
    if (ch >= 'a' && ch <= 'f')
        return 10 + ch - 'a';

    // A-F
    if (ch >= 'A' && ch <= 'F')
        return 10 + ch - 'A';

    return 0;
}

template<typename T>
static constexpr T ParseHexDigits(const char*& str)
{
    constexpr uint32_t numBytes = sizeof(T) * 2;
    constexpr uint32_t bitsPerByte = 4;

    T bytes{};
    for (uint32_t i = 1; i <= numBytes; ++i)
    {
        bytes |= HexDigitToByte(*str) << (bitsPerByte * (numBytes - i));
        str++;
    }

    if (*str == '-')
    {
        str++;
    }

    return bytes;
}

static constexpr bool IsValidHexChar(char ch)
{
    // 0-9
    if (ch > 47 && ch < 58)
        return true;

    // a-f
    if (ch > 96 && ch < 103)
        return true;

    // A-F
    if (ch > 64 && ch < 71)
        return true;

    return false;
}

class Guid
{
    friend struct std::hash<Gleam::Guid>;
    
public:

	static Guid NewGuid();
	static constexpr Guid InvalidGuid()
	{
		return Guid();
	}
    static Guid Combine(const Guid& guid1, const Guid& guid2);

	Guid() = default;
	Guid(const TStringView str);
    Guid(const Reflection::Attribute::Guid& guid);
    
    Guid& operator=(const Guid&) = default;
    Guid& operator=(const Reflection::Attribute::Guid& guid);

    TString ToString() const;
	operator TString() const
    {
        return ToString();
    }

	bool operator==(const Guid& other) const;
	bool operator!=(const Guid& other) const;
    
    const TArray<uint8_t, 16>& GetBytes() const { return mBytes; }

private:

	union
	{
		struct
		{
			uint32_t mData1;
			uint16_t mData2;
			uint16_t mData3;
			uint8_t mData4[8];
		};
		TArray<uint8_t, 16> mBytes = { 0 };
	};

};

} // namespace Gleam

template <>
struct std::hash<Gleam::Guid>
{
    size_t operator()(const Gleam::Guid& guid) const
    {
        size_t hash = 0;
        Gleam::hash_combine(hash, guid.mData1);
        Gleam::hash_combine(hash, guid.mData2);
        Gleam::hash_combine(hash, guid.mData3);
        
        uint64_t data4;
        memcpy(&data4, guid.mData4, sizeof(uint64_t));
        Gleam::hash_combine(hash, data4);
        return hash;
    }
};
