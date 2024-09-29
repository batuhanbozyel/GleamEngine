#pragma once

#ifndef RAPIDJSON_HAS_STDSTRING
#define RAPIDJSON_HAS_STDSTRING 1
#endif

#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/ostreamwrapper.h>

namespace rapidjson {

struct Node
{
    Value& object;
    Document::AllocatorType& allocator;
    
    template <typename T>
    RAPIDJSON_DISABLEIF_RETURN((internal::OrExpr<internal::IsPointer<T>, internal::IsGenericValue<T> >), (Node&))
    AddMember(Value::StringRefType name, T value)
    {
        Value n(name);
        object.AddMember(n, value, allocator);
        return *this;
    }
    
    Node& AddMember(Value::StringRefType name, Value& value)
    {
        Value n(name);
        object.AddMember(n, value, allocator);
        return *this;
    }
    
    Node& AddMember(Value::StringRefType name, Value::StringRefType value)
    {
        Value v(value);
        object.AddMember(name, v, allocator);
        return *this;
    }
    
    Node& AddMember(Value::StringRefType name, Value&& value)
    {
        Value n(name);
        object.AddMember(n, value, allocator);
        return *this;
    }
    
    Node& PushBack(Value&& value)
    {
        object.PushBack(std::move(value), allocator);
        return *this;
    }
    
    Node& PushBack(Value& value)
    {
        object.PushBack(value, allocator);
        return *this;
    }

	Value& operator[](const char* name) const
	{
		return object[name];
	}
    
    explicit Node(Value& object, Document::AllocatorType& allocator)
        : object(object), allocator(allocator)
    {
        
    }
};

struct ConstNode
{
	const Value& object;

	explicit ConstNode(const Value& object)
		: object(object)
	{

	}

	const Value& operator[](const char* name) const
	{
		return object[name];
	}
};

static Gleam::TString Convert(const rapidjson::Value& value)
{
    StringBuffer buffer;
    PrettyWriter<StringBuffer> writer(buffer);
	writer.SetFormatOptions(PrettyFormatOptions::kFormatSingleLineArray);
	writer.SetMaxDecimalPlaces(6);
    writer.SetIndent('\t', 1);
    value.Accept(writer);
    return buffer.GetString();
}

namespace internal {

template<typename ValueType>
struct TypeHelper<ValueType, char>
{
	static bool Is(const ValueType& v) { return v.IsUint(); }
	static char Get(const ValueType& v) { return static_cast<char>(v.GetUint()); }
	static ValueType& Set(ValueType& v, char data) { return v.SetUint(static_cast<unsigned int>(data)); }
	static ValueType& Set(ValueType& v, char data, typename ValueType::AllocatorType&) { return v.SetUint(static_cast<unsigned int>(data)); }
};

template<typename ValueType>
struct TypeHelper<ValueType, wchar_t>
{
	static bool Is(const ValueType& v) { return v.IsUint(); }
	static wchar_t Get(const ValueType& v) { return static_cast<wchar_t>(v.GetUint()); }
	static ValueType& Set(ValueType& v, wchar_t data) { return v.SetUint(static_cast<unsigned int>(data)); }
	static ValueType& Set(ValueType& v, wchar_t data, typename ValueType::AllocatorType&) { return v.SetUint(static_cast<unsigned int>(data)); }
};

template<typename ValueType>
struct TypeHelper<ValueType, int8_t>
{
	static bool Is(const ValueType& v) { return v.IsInt(); }
	static int8_t Get(const ValueType& v) { return static_cast<int8_t>(v.GetInt()); }
	static ValueType& Set(ValueType& v, int8_t data) { return v.SetInt(static_cast<int>(data)); }
	static ValueType& Set(ValueType& v, int8_t data, typename ValueType::AllocatorType&) { return v.SetInt(static_cast<int>(data)); }
};

template<typename ValueType>
struct TypeHelper<ValueType, uint8_t>
{
	static bool Is(const ValueType& v) { return v.IsUint(); }
	static uint8_t Get(const ValueType& v) { return static_cast<uint8_t>(v.GetUint()); }
	static ValueType& Set(ValueType& v, uint8_t data) { return v.SetUint(static_cast<unsigned int>(data)); }
	static ValueType& Set(ValueType& v, uint8_t data, typename ValueType::AllocatorType&) { return v.SetUint(static_cast<unsigned int>(data)); }
};

template<typename ValueType>
struct TypeHelper<ValueType, int16_t>
{
	static bool Is(const ValueType& v) { return v.IsInt(); }
	static int16_t Get(const ValueType& v) { return static_cast<int16_t>(v.GetInt()); }
	static ValueType& Set(ValueType& v, int16_t data) { return v.SetInt(static_cast<int>(data)); }
	static ValueType& Set(ValueType& v, int16_t data, typename ValueType::AllocatorType&) { return v.SetInt(static_cast<int>(data)); }
};

template<typename ValueType>
struct TypeHelper<ValueType, uint16_t>
{
	static bool Is(const ValueType& v) { return v.IsUint(); }
	static uint16_t Get(const ValueType& v) { return static_cast<uint16_t>(v.GetUint()); }
	static ValueType& Set(ValueType& v, uint16_t data) { return v.SetUint(static_cast<unsigned int>(data)); }
	static ValueType& Set(ValueType& v, uint16_t data, typename ValueType::AllocatorType&) { return v.SetUint(static_cast<unsigned int>(data)); }
};

}

} // namespace rapidjson
