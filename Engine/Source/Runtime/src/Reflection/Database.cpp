#include "gpch.h"
#include "Database.h"

// This should never be used, but it is here to avoid returning reference to local variable
struct Dummy {};
GLEAM_TYPE(Dummy, Guid("00000000-0000-0000-0000-000000000000"))
GLEAM_END

// This should never be used, but it is here to avoid returning reference to local variable
enum class DummyEnum {};
GLEAM_TYPE(DummyEnum, Guid("00000000-0000-0000-0000-000000000000"))
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
    
    
    mPrimitiveTypes[typeid(bool).hash_code()] = PrimitiveType::Bool;
    mPrimitiveTypes[typeid(wchar_t).hash_code()] = PrimitiveType::WChar;
    mPrimitiveTypes[typeid(char).hash_code()] = PrimitiveType::Char;
    mPrimitiveTypes[typeid(int8_t).hash_code()] = PrimitiveType::Int8;
    mPrimitiveTypes[typeid(int16_t).hash_code()] = PrimitiveType::Int16;
    mPrimitiveTypes[typeid(int32_t).hash_code()] = PrimitiveType::Int32;
    mPrimitiveTypes[typeid(int64_t).hash_code()] = PrimitiveType::Int64;
    mPrimitiveTypes[typeid(uint8_t).hash_code()] = PrimitiveType::UInt8;
    mPrimitiveTypes[typeid(uint16_t).hash_code()] = PrimitiveType::UInt16;
    mPrimitiveTypes[typeid(uint32_t).hash_code()] = PrimitiveType::UInt32;
    mPrimitiveTypes[typeid(uint64_t).hash_code()] = PrimitiveType::UInt64;
    mPrimitiveTypes[typeid(float).hash_code()] = PrimitiveType::Float;
    mPrimitiveTypes[typeid(double).hash_code()] = PrimitiveType::Double;
    mPrimitiveTypes[typeid(void).hash_code()] = PrimitiveType::Void;
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
	GLEAM_ASSERT(false, "Invalid class hash.");
	static auto type = refl::reflect<Dummy>();
	static auto dummy = CreateClassDescription(type);
	return dummy;
}

const EnumDescription& Database::GetEnum(size_t hash)
{
    auto it = mEnums.find(hash);
    if (it != mEnums.end())
    {
        return it->second;
    }
    GLEAM_ASSERT(false, "Invalid enum hash.");
    static auto type = refl::reflect<DummyEnum>();
	static auto dummy = CreateEnumDescription(type);
	return dummy;
}

const ArrayDescription& Database::GetArray(size_t hash)
{
    auto it = mArrays.find(hash);
    if (it != mArrays.end())
    {
        return it->second;
    }
    GLEAM_ASSERT(false, "Invalid array hash.");
	static auto dummy = CreateArrayDescription<Dummy[1]>();
	return dummy;
}

size_t Database::GetTypeHash(const TStringView name)
{
	auto it = mTypeNameToHash.find(name);
	if (it != mTypeNameToHash.end())
	{
		return it->second;
	}
	GLEAM_ASSERT(false, "Invalid type name.");
	return 0;
}
