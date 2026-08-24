#include "gpch.h"
#include "AssetDataLayout.h"

#include "Core/GUID.h"

#include "IO/Log.h"

using namespace Gleam;

static uint64_t HashClassLayout(const Reflection::ClassDescription& classDesc)
{
	size_t seed = 0;
	hash_combine(seed, Guid(classDesc.Guid()));
	hash_combine(seed, classDesc.GetSize());

	for (const auto& baseClass : classDesc.ResolveBaseClasses())
	{
		hash_combine(seed, HashClassLayout(baseClass));
	}

	for (const auto& field : classDesc.ResolveFields())
	{
		hash_combine(seed, Guid(field.Guid()));
		hash_combine(seed, field.GetOffset());
		hash_combine(seed, field.GetSize());
		hash_combine(seed, ResolveTypeLayoutHash(field.GetType(), field.TypeHash()));
	}
	return static_cast<uint64_t>(seed);
}

uint64_t Gleam::ResolveTypeLayoutHash(Reflection::MetaType type, uint32_t typeHash)
{
	size_t seed = 0;
	hash_combine(seed, static_cast<uint32_t>(type));

	switch (type)
	{
		case Reflection::MetaType::Primitive:
		{
			const auto primitive = Reflection::GetPrimitive(typeHash);
			hash_combine(seed, static_cast<uint32_t>(primitive.Type()));
			hash_combine(seed, primitive.GetSize());
			break;
		}
		case Reflection::MetaType::Enum:
		{
			const auto enumDesc = Reflection::GetEnum(typeHash);
			hash_combine(seed, Guid(enumDesc->Guid()));
			hash_combine(seed, enumDesc->GetSize());
			break;
		}
		case Reflection::MetaType::Class:
		{
			hash_combine(seed, HashClassLayout(*Reflection::GetClass(typeHash)));
			break;
		}
		default:
		{
			GLEAM_ASSERT(false, "Blob layout hash: unsupported element type.");
			break;
		}
	}
	return static_cast<uint64_t>(seed);
}
