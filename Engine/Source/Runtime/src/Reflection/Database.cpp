#include "gpch.h"
#include "Database.h"

// This should never be used, but it is here to avoid returning reference to local variable
struct NullType {};
GLEAM_TYPE(NullType, Guid("00000000-0000-0000-0000-000000000000"))
GLEAM_END

// This should never be used, but it is here to avoid returning reference to local variable
enum class NullEnum {};
GLEAM_TYPE(NullEnum, Guid("00000000-0000-0000-0000-000000000000"))
GLEAM_END

using namespace Gleam::Reflection;

void Database::Initialize(Engine* engine)
{
    mPrimitiveNames[PrimitiveType::Bool] = refl::reflect<bool>().name.c_str();
    mPrimitiveNames[PrimitiveType::WChar] = refl::reflect<wchar_t>().name.c_str();
    mPrimitiveNames[PrimitiveType::Char] = refl::reflect<char>().name.c_str();
    mPrimitiveNames[PrimitiveType::Int8] = refl::reflect<int8_t>().name.c_str();
    mPrimitiveNames[PrimitiveType::Int16] = refl::reflect<int16_t>().name.c_str();
    mPrimitiveNames[PrimitiveType::Int32] = refl::reflect<int32_t>().name.c_str();
    mPrimitiveNames[PrimitiveType::Int64] = refl::reflect<int64_t>().name.c_str();
    mPrimitiveNames[PrimitiveType::UInt8] = refl::reflect<uint8_t>().name.c_str();
    mPrimitiveNames[PrimitiveType::UInt16] = refl::reflect<uint16_t>().name.c_str();
    mPrimitiveNames[PrimitiveType::UInt32] = refl::reflect<uint32_t>().name.c_str();
    mPrimitiveNames[PrimitiveType::UInt64] = refl::reflect<uint64_t>().name.c_str();
    mPrimitiveNames[PrimitiveType::Float] = refl::reflect<float>().name.c_str();
    mPrimitiveNames[PrimitiveType::Double] = refl::reflect<double>().name.c_str();
    mPrimitiveNames[PrimitiveType::Void] = refl::reflect<void>().name.c_str();
    
    
	mPrimitiveTypes[entt::type_hash<bool>::value()] = PrimitiveType::Bool;
	mPrimitiveTypes[entt::type_hash<wchar_t>::value()] = PrimitiveType::WChar;
	mPrimitiveTypes[entt::type_hash<char>::value()] = PrimitiveType::Char;
	mPrimitiveTypes[entt::type_hash<int8_t>::value()] = PrimitiveType::Int8;
	mPrimitiveTypes[entt::type_hash<int16_t>::value()] = PrimitiveType::Int16;
	mPrimitiveTypes[entt::type_hash<int32_t>::value()] = PrimitiveType::Int32;
	mPrimitiveTypes[entt::type_hash<int64_t>::value()] = PrimitiveType::Int64;
	mPrimitiveTypes[entt::type_hash<uint8_t>::value()] = PrimitiveType::UInt8;
	mPrimitiveTypes[entt::type_hash<uint16_t>::value()] = PrimitiveType::UInt16;
	mPrimitiveTypes[entt::type_hash<uint32_t>::value()] = PrimitiveType::UInt32;
	mPrimitiveTypes[entt::type_hash<uint64_t>::value()] = PrimitiveType::UInt64;
	mPrimitiveTypes[entt::type_hash<float>::value()] = PrimitiveType::Float;
	mPrimitiveTypes[entt::type_hash<double>::value()] = PrimitiveType::Double;
	mPrimitiveTypes[entt::type_hash<void>::value()] = PrimitiveType::Void;
}

void Database::Shutdown()
{
    mPrimitiveNames.clear();
    mPrimitiveTypes.clear();
}

PrimitiveType Database::GetPrimitiveType(size_t hash)
{
    auto it = mPrimitiveTypes.find(hash);
    if (it != mPrimitiveTypes.end())
    {
        return it->second;
    }
	GLEAM_ASSERT(false, "Invalid primitive hash.");
    return PrimitiveType::Invalid;
}

const Gleam::TStringView Database::GetPrimitiveName(PrimitiveType type)
{
    auto it = mPrimitiveNames.find(type);
    if (it != mPrimitiveNames.end())
    {
        return it->second;
    }
	GLEAM_ASSERT(false, "Invalid primitive type.");
    return "";
}

const ClassDescription& Database::GetClass(size_t hash)
{
	auto it = mClasses.find(hash);
	if (it != mClasses.end())
	{
		return it->second;
	}
	GLEAM_WARN("Invalid class hash.");
	static auto type = refl::reflect<NullType>();
	static auto null = CreateClassDescription(type);
	return null;
}

const EnumDescription& Database::GetEnum(size_t hash)
{
    auto it = mEnums.find(hash);
    if (it != mEnums.end())
    {
        return it->second;
    }
	GLEAM_WARN("Invalid enum hash.");
    static auto type = refl::reflect<NullEnum>();
	static auto null = CreateEnumDescription(type);
	return null;
}

const ArrayDescription& Database::GetArray(size_t hash)
{
    auto it = mArrays.find(hash);
    if (it != mArrays.end())
    {
        return it->second;
    }
	GLEAM_WARN("Invalid array hash.");
	static auto null = CreateArrayDescription<NullType[1]>();
	return null;
}

size_t Database::GetTypeHash(const TStringView name)
{
	auto it = mTypeNameToHash.find(name);
	if (it != mTypeNameToHash.end())
	{
		return it->second;
	}
	GLEAM_WARN("Invalid type name.");
	return 0;
}
