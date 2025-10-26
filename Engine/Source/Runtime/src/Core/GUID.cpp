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
    for (int i = 0; i < 16; i++)
    {
        combined.mBytes[i] = guid1.mBytes[i] ^ guid2.mBytes[i];
    }
    return combined;
}
