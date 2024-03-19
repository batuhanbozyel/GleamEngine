#include "gpch.h"
#include "GUID.h"

#if defined(PLATFORM_WINDOWS)
#include <objbase.h>
Gleam::Guid Gleam::Guid::NewGuid()
{
	GUID uuid;
	CoCreateGuid(&uuid);

    Gleam::Guid guid;
	guid.mBytes =
	{
		(uint8_t)((uuid.Data1 >> 24) & 0xFF),
		(uint8_t)((uuid.Data1 >> 16) & 0xFF),
		(uint8_t)((uuid.Data1 >> 8) & 0xFF),
		(uint8_t)((uuid.Data1) & 0xff),

		(uint8_t)((uuid.Data2 >> 8) & 0xFF),
		(uint8_t)((uuid.Data2) & 0xff),

		(uint8_t)((uuid.Data3 >> 8) & 0xFF),
		(uint8_t)((uuid.Data3) & 0xFF),

		(uint8_t)uuid.Data4[0],
		(uint8_t)uuid.Data4[1],
		(uint8_t)uuid.Data4[2],
		(uint8_t)uuid.Data4[3],
		(uint8_t)uuid.Data4[4],
		(uint8_t)uuid.Data4[5],
		(uint8_t)uuid.Data4[6],
		(uint8_t)uuid.Data4[7]
	};
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

static constexpr uint8_t HexDigitToByte(char ch)
{
	// 0-9
	if (ch > 47 && ch < 58)
		return ch - 48;

	// a-f
	if (ch > 96 && ch < 103)
		return ch - 87;

	// A-F
	if (ch > 64 && ch < 71)
		return ch - 55;

	return 0;
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

static constexpr uint8_t HexPairToByte(char a, char b)
{
	return HexDigitToByte(a) * 16 + HexDigitToByte(b);
}

Guid::Guid(const TStringView str)
{
	if (str.length() != 20) // 16 bytes + 4 seperators
	{
		mBytes = InvalidGuid.mBytes;
		GLEAM_ASSERT(false, "Invalid Guid string!");
		return;
	}

	char charOne = '\0';
	char charTwo = '\0';
	bool lookingForFirstChar = true;
	uint32_t nextByte = 0;

	for (const char ch : str)
	{
		if (ch == '-')
			continue;

		if (IsValidHexChar(ch) == false)
		{
			mBytes = InvalidGuid.mBytes;
			GLEAM_ASSERT(false, "Invalid Guid string!");
			return;
		}
		
		if (lookingForFirstChar)
		{
			charOne = ch;
			lookingForFirstChar = false;
		}
		else
		{
			charTwo = ch;
			auto byte = HexPairToByte(charOne, charTwo);
			mBytes[nextByte++] = byte;
			lookingForFirstChar = true;
		}
	}
}

Guid::operator TString() const
{
	char one[10], two[6], three[6], four[6], five[14];

	snprintf(one, 10, "%02x%02x%02x%02x", mBytes[0], mBytes[1], mBytes[2], mBytes[3]);
	snprintf(two, 6, "%02x%02x", mBytes[4], mBytes[5]);
	snprintf(three, 6, "%02x%02x", mBytes[6], mBytes[7]);
	snprintf(four, 6, "%02x%02x", mBytes[8], mBytes[9]);
	snprintf(five, 14, "%02x%02x%02x%02x%02x%02x", mBytes[10], mBytes[11], mBytes[12], mBytes[13], mBytes[14], mBytes[15]);

	TStringStream out;
	out << one;
	out << '-' << two;
	out << '-' << three;
	out << '-' << four;
	out << '-' << five;
	return out.str();
}

bool Guid::operator==(const Guid &other) const
{
	return mBytes == other.mBytes;
}

bool Guid::operator!=(const Guid &other) const
{
	return !((*this) == other);
}

std::ostream& operator<<(std::ostream& s, const Guid& guid)
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
