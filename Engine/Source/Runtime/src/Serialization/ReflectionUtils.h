#pragma once
#include "IO/Log.h"

#include <Reflection/Reflection.h>

#include <cstring>
#include <cstdint>

namespace Gleam::ReflectionUtils {

inline const Reflection::EnumCaseDescription* FindCase(const Reflection::EnumDescription& enumDesc, int64_t value)
{
	for (const auto& enumCase : enumDesc.Cases())
	{
		if (enumCase.Value() == value)
		{
			return &enumCase;
		}
	}
	return nullptr;
}

inline const Reflection::EnumCaseDescription* FindCase(const Reflection::EnumDescription& enumDesc,
													   const Reflection::Attribute::Guid& guid)
{
	for (const auto& enumCase : enumDesc.Cases())
	{
		if (enumCase.Guid() == guid)
		{
			return &enumCase;
		}
	}
	return nullptr;
}

inline int64_t ReadEnumValue(const void* obj, const Reflection::EnumDescription& enumDesc)
{
	int64_t value = 0;
	memcpy(&value, obj, enumDesc.GetSize());
	return value;
}

inline void WriteEnumValue(void* obj, const Reflection::EnumDescription& enumDesc, int64_t value)
{
	memcpy(obj, &value, enumDesc.GetSize());
}

inline Reflection::Attribute::Guid ResolveCaseGuid(const void* obj, const Reflection::EnumDescription& enumDesc)
{
	const auto value = ReadEnumValue(obj, enumDesc);
	const auto enumCase = FindCase(enumDesc, value);
	GLEAM_ASSERT(enumCase != nullptr, "Serialization: enum value does not match any case. Bitmask enums are not supported.");

	if (enumCase != nullptr)
	{
		return enumCase->Guid();
	}
	else
	{
		return Reflection::Attribute::Guid::InvalidGuid();
	}
}

} // namespace Gleam::ReflectionUtils
