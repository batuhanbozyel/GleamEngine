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

inline uint64_t TruncateToSize(int64_t value, size_t size)
{
	const auto bits = size * 8;
	const auto sizeMask = bits >= 64 ? ~0ull : (1ull << bits) - 1ull;
	return static_cast<uint64_t>(value) & sizeMask;
}

inline const Reflection::EnumDescription* ResolveFlagEnum(const Reflection::ClassDescription& classDesc)
{
	const auto templateParams = classDesc.ResolveTemplateParameters();
	GLEAM_ASSERT(templateParams.size() == 1 and templateParams[0].GetType() == Reflection::MetaType::Enum,
				 "Serialization: EnumFlag must have exactly one reflected enum template parameter.");
	return Reflection::GetEnum(templateParams[0].TypeHash());
}

inline uint64_t ReadFlagMask(const void* obj, size_t size)
{
	uint64_t mask = 0;
	memcpy(&mask, obj, size);
	return mask;
}

inline void WriteFlagMask(void* obj, size_t size, uint64_t mask)
{
	memcpy(obj, &mask, size);
}

template<typename Function>
inline void ForEachSetCase(uint64_t mask, const Reflection::EnumDescription& enumDesc, Function&& function)
{
	for (const auto& enumCase : enumDesc.Cases())
	{
		const auto caseMask = TruncateToSize(enumCase.Value(), enumDesc.GetSize());
		if (caseMask != 0 and (mask & caseMask) == caseMask)
		{
			function(enumCase);
		}
	}
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
	GLEAM_ASSERT(enumCase != nullptr, "Serialization: enum value does not match any case. Bitmask enums are not supported, use EnumFlag instead.");

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
