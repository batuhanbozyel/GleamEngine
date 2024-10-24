#include "gpch.h"
#include "GUID.h"

#if defined(PLATFORM_WINDOWS)
#include <objbase.h>
Gleam::Guid Gleam::Guid::NewGuid()
{
	Gleam::Guid guid = Gleam::Guid::InvalidGuid();
	if (SUCCEEDED(CoCreateGuid((GUID*)&guid))) {} // to emit warning
	return guid;
}
#elif defined(PLATFORM_MACOS)
#include <CoreFoundation/CFUUID.h>
Gleam::Guid Gleam::Guid::NewGuid()
{
	auto uuid = CFUUIDCreate(NULL);
	auto bytes = CFUUIDGetUUIDBytes(uuid);
	CFRelease(uuid);

    Gleam::Guid guid;
	guid.mBytes =
	{
		bytes.byte0,
		bytes.byte1,
		bytes.byte2,
		bytes.byte3,
		bytes.byte4,
		bytes.byte5,
		bytes.byte6,
		bytes.byte7,
		bytes.byte8,
		bytes.byte9,
		bytes.byte10,
		bytes.byte11,
		bytes.byte12,
		bytes.byte13,
		bytes.byte14,
		bytes.byte15
	};
	return guid;
}
#endif

using namespace Gleam;

Guid Guid::Combine(const Guid& guid1, const Guid& guid2)
{
    Guid combined;
    for (int i = 0; i < combined.mBytes.size(); i++)
    {
        combined.mBytes[i] = guid1.mBytes[i] ^ guid2.mBytes[i];
    }
    return combined;
}

Guid::Guid(const TString& str)
	: mBytes({ 0 })
{
	if (str.length() < 20) // 16 bytes + 4 separators
	{
		*this = Guid::InvalidGuid();
		GLEAM_ASSERT(false, "Invalid Guid string!");
		return;
	}

	auto it = str.data();
	if (*it == '{')
	{
		it++;
	}

	// {mData1-mData2-mData3-mData4[0,1]-mData4[2,7]}
	// {XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX}
	mData1 = ParseHexDigits<uint32_t>(it);
	mData2 = ParseHexDigits<uint16_t>(it);
	mData3 = ParseHexDigits<uint16_t>(it);

	mData4[0] = ParseHexDigits<uint8_t>(it);
	mData4[1] = ParseHexDigits<uint8_t>(it);

	mData4[2] = ParseHexDigits<uint8_t>(it);
	mData4[3] = ParseHexDigits<uint8_t>(it);
	mData4[4] = ParseHexDigits<uint8_t>(it);
	mData4[5] = ParseHexDigits<uint8_t>(it);
	mData4[6] = ParseHexDigits<uint8_t>(it);
	mData4[7] = ParseHexDigits<uint8_t>(it);
}

Guid::Guid(const Reflection::Attribute::Guid& guid)
	: mBytes(guid.bytes)
{
    
}

Guid& Guid::operator=(const Reflection::Attribute::Guid& guid)
{
    mBytes = guid.bytes;
	return *this;
}

TString Guid::ToString() const
{
    TString str;
    str.resize(36);
    snprintf(str.data(),
             str.length() + 1,
             "%08X-%04hX-%04hX-%02hhX%02hhX-%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX",
             mData1,
             mData2,
             mData3,
             mData4[0],
             mData4[1],
             mData4[2],
             mData4[3],
             mData4[4],
             mData4[5],
             mData4[6],
             mData4[7]);
    return str;
}

bool Guid::operator==(const Guid& other) const
{
	return mBytes == other.mBytes;
}

bool Guid::operator!=(const Guid& other) const
{
	return !((*this) == other);
}

static std::ostream& operator<<(std::ostream& s, const Guid& guid)
{
	std::ios_base::fmtflags f(s.flags());
    const auto& bytes = guid.GetBytes();
	s << std::hex << std::setfill('0')
		<< std::setw(2) << (int)bytes[0]
		<< std::setw(2) << (int)bytes[1]
		<< std::setw(2) << (int)bytes[2]
		<< std::setw(2) << (int)bytes[3]
		<< "-"
		<< std::setw(2) << (int)bytes[4]
		<< std::setw(2) << (int)bytes[5]
		<< "-"
		<< std::setw(2) << (int)bytes[6]
		<< std::setw(2) << (int)bytes[7]
		<< "-"
		<< std::setw(2) << (int)bytes[8]
		<< std::setw(2) << (int)bytes[9]
		<< "-"
		<< std::setw(2) << (int)bytes[10]
		<< std::setw(2) << (int)bytes[11]
		<< std::setw(2) << (int)bytes[12]
		<< std::setw(2) << (int)bytes[13]
		<< std::setw(2) << (int)bytes[14]
		<< std::setw(2) << (int)bytes[15];
	s.flags(f);
	return s;
}
