#pragma once
#include "Attribute.h"

namespace Gleam::Reflection {

enum class FieldType
{
    Primitive,
    Array,
    Class,
    Enum,
    Invalid
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

struct PrimitiveField
{
    static constexpr FieldType type = FieldType::Primitive;
    
    PrimitiveType primitive;
};

struct ArrayField
{
    static constexpr FieldType type = FieldType::Array;
    
    uint32_t stride;
    uint32_t count;
};

struct ClassField
{
    static constexpr FieldType type = FieldType::Class;
};

struct EnumField
{
    static constexpr FieldType type = FieldType::Enum;
};

using Field = std::variant<PrimitiveField, ArrayField, ClassField, EnumField>;

class FieldDescription
{
public:
    template<typename T, size_t N>
    explicit constexpr FieldDescription(const refl::field_descriptor<T, N>& field)
        : mName(field.name.c_str())
        , mSize(sizeof(T))
    {
        if constexpr (std::is_fundamental<T>::value)
        {
            mType = FieldType::Primitive;
        }
        else if (std::is_array<T>::value)
        {
            mType = FieldType::Array;
        }
        else if (std::is_class<T>::value)
        {
            mType = FieldType::Class;
        }
        else if (std::is_enum<T>::value)
        {
            mType = FieldType::Enum;
        }
        else
        {
            mType = FieldType::Invalid;
        }
    }
    
    template<typename T>
        requires(std::is_same<typename T::type, FieldType>::value)
    constexpr const T& GetField() const
    {
        static_assert(T::type == mType, "Requested field type does not match!");
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
    bool HasAttribute() const
    {
        
    }
    
    template<AttributeType Attrib>
    const auto& GetAttribute() const
    {
        
    }
    
private:
    
    size_t mSize;
    Field mField;
    FieldType mType;
    TStringView mName;
};

class ClassDescription
{
public:
    template<typename T>
    explicit constexpr ClassDescription(const refl::type_descriptor<T>& type)
        : mGuid(refl::descriptor::get_attribute<Reflection::Guid>(type))
        , mName(type.name.c_str())
    {
        // resolve fields
        auto fields = refl::util::filter(type.members, [](auto member) { return refl::descriptor::is_field(member); });
        refl::util::for_each(fields, [&](auto member)
        {
            mFields.emplace_back(member);
        });
        
        // resolve base classes
        refl::util::for_each(type.bases, [&](auto base)
        {
            mBaseClasses.emplace_back(base);
        });
    }
    
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
    bool HasAttribute() const noexcept
    {
        
    }
    
    template<AttributeType Attrib>
    const auto& GetAttribute() const noexcept
    {
        
    }
    
private:
    
    Gleam::Guid mGuid;
    TStringView mName;
    TArray<FieldDescription> mFields;
    TArray<ClassDescription> mBaseClasses;
    TArray<AttributeDescription> mAttributes;
};

} // namespace Gleam::Reflection
