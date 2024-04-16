#pragma once
#include "Attribute.h"
#include "TypeTraits.h"

#include <variant>
#include <any>

namespace Gleam::Reflection {

class Database;

struct AttributePair
{
    AttributeDescription description;
    std::any value;
};

enum class FieldType
{
    Invalid,
    Primitive,
    Array,
    Class,
    Enum
};

enum class PrimitiveType
{
    Invalid,
    Bool,
    WChar,
    Char,
    Int8,
    Int16,
    Int32,
    Int64,
    UInt8,
    UInt16,
    UInt32,
    UInt64,
    Float,
    Double,
    Void,

    COUNT
};

template<FieldType T>
struct FieldBase
{
    static constexpr FieldType type = T;
    
    size_t size;
    size_t offset;
};

struct NullField : FieldBase<FieldType::Invalid> {};

struct PrimitiveField : FieldBase<FieldType::Primitive>
{
    PrimitiveType primitive;
    
    explicit constexpr PrimitiveField(PrimitiveType primitive)
        : primitive(primitive)
    {
        
    }
};

struct ArrayField : FieldBase<FieldType::Array>
{
    size_t hash;
    
    explicit constexpr ArrayField(size_t hash)
        : FieldBase(), hash(hash)
    {
        
    }
};

struct ClassField : FieldBase<FieldType::Class>
{
    size_t hash;
    
    explicit constexpr ClassField(size_t hash)
        : FieldBase(), hash(hash)
    {
        
    }
};

struct EnumField : FieldBase<FieldType::Enum>
{
    size_t hash;
    
    explicit constexpr EnumField(size_t hash)
        : FieldBase(), hash(hash)
    {
        
    }
};

// IMPORTANT: the order needs to match with FieldType enum items order
using Field = std::variant<NullField,
                           PrimitiveField,
                           ArrayField,
                           ClassField,
                           EnumField>;

class FieldDescription
{
	friend class Database;
public:
    
    template<typename T>
    constexpr const T& GetField() const
    {
        GLEAM_ASSERT(T::type == mType, "Requested field type does not match!");
        return std::get<static_cast<uint32_t>(T::type)>(mField);
    }
    
    constexpr const TStringView ResolveName() const
    {
        return mName;
    }
    
    constexpr FieldType GetType() const
    {
        return mType;
    }
    
    template<AttributeType Attrib>
    constexpr bool HasAttribute() const
    {
        for (const auto& attrib : mAttributes)
        {
            if (attrib.description.hash == Attrib::description.hash)
            {
                return true;
            }
        }
        return false;
    }
    
    template<AttributeType Attrib>
    constexpr Attrib GetAttribute() const
    {
        for (const auto& attrib : mAttributes)
        {
            if (attrib.description.hash == Attrib::description.hash)
            {
                return std::any_cast<Attrib>(attrib.value);
            }
        }
        return Attrib({});
    }
    
private:
    
    Field mField;
    FieldType mType;
    TStringView mName;
    TArray<AttributePair> mAttributes;
};

class EnumDescription
{
    friend class Database;
public:
    
    constexpr const Gleam::Guid& Guid() const
    {
        return mGuid;
    }
    
    constexpr const TStringView ResolveName() const
    {
        return mName;
    }
    
    template<AttributeType Attrib>
    constexpr bool HasAttribute() const
    {
        for (const auto& attrib : mAttributes)
        {
            if (attrib.description.hash == Attrib::description.hash)
            {
                return true;
            }
        }
        return false;
    }
    
    template<AttributeType Attrib>
    constexpr Attrib GetAttribute() const
    {
        for (const auto& attrib : mAttributes)
        {
            if (attrib.description.hash == Attrib::description.hash)
            {
                return std::any_cast<Attrib>(attrib.value);
            }
        }
        return Attrib({});
    }
    
private:
    
    Gleam::Guid mGuid;
    TStringView mName;
    TArray<AttributePair> mAttributes;
    
};

class ClassDescription
{
	friend class Database;
public:

    constexpr const Gleam::Guid& Guid() const
    {
        return mGuid;
    }
    
    constexpr const TStringView ResolveName() const
    {
        return mName;
    }
    
    constexpr const TArray<FieldDescription>& ResolveFields() const
    {
        return mFields;
    }
    
    constexpr const TArray<ClassDescription>& ResolveBaseClasses() const
    {
        return mBaseClasses;
    }
    
    template<AttributeType Attrib>
    constexpr bool HasAttribute() const
    {
        for (const auto& attrib : mAttributes)
        {
            if (attrib.description.hash == Attrib::description.hash)
            {
                return true;
            }
        }
        return false;
    }
    
    template<AttributeType Attrib>
    constexpr Attrib GetAttribute() const
    {
        for (const auto& attrib : mAttributes)
        {
            if (attrib.description.hash == Attrib::description.hash)
            {
                return std::any_cast<Attrib>(attrib.value);
            }
        }
        return Attrib({});
    }
    
    constexpr size_t GetSize() const
    {
        return mSize;
    }
    
    constexpr size_t ContainerHash() const
    {
        return mContainerHash;
    }
    
private:
    
    size_t mSize;
    Gleam::Guid mGuid;
    TStringView mName;
    TArray<FieldDescription> mFields;
    TArray<ClassDescription> mBaseClasses;
    TArray<AttributePair> mAttributes;
    
    size_t mContainerHash;
};

class ArrayDescription
{
    friend class Database;
public:
    
    ArrayDescription() = default;
    ArrayDescription(const ArrayDescription&) = default;
    ArrayDescription(const TStringView name, FieldType type, size_t hash, size_t size, size_t stride)
        : mName(name), mType(type), mHash(hash), mSize(size), mStride(stride)
    {
        
    }
    
    constexpr const TStringView ResolveName() const
    {
        return mName;
    }
    
    constexpr size_t GetSize() const
    {
        return mSize;
    }
    
    constexpr size_t GetStride() const
    {
        return mStride;
    }
    
    constexpr size_t ElementHash() const
    {
        return mHash;
    }
    
    constexpr FieldType ElementType() const
    {
        return mType;
    }
    
private:
    
    size_t mHash;
    size_t mSize;
    size_t mStride;
    FieldType mType;
    TStringView mName;
    
};

} // namespace Gleam::Reflection
