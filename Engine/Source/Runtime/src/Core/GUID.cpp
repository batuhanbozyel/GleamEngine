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
	memcpy(guid.mBytes, &bytes.byte0, sizeof(bytes));
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
