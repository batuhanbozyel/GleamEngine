#pragma once
#include "Attribute.h"

#include <entt/meta/factory.hpp>
#include <variant>

static constexpr uint32_t operator"" _hs(const char* str, size_t size)
{
    return entt::hashed_string(str);
}

namespace Gleam::Reflection {

struct AttributePair
{
    AttributeDescription description;
    std::any value;
};

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

template<FieldType T>
struct FieldBase
{
    static constexpr FieldType type = T;
    
    size_t size;
    size_t offset;
};

struct PrimitiveField : FieldBase<FieldType::Primitive>
{
    PrimitiveType primitive;
};

struct ArrayField : FieldBase<FieldType::Array>
{
    FieldType elementType;
    uint32_t stride;
    uint32_t count;
};

struct ClassField : FieldBase<FieldType::Class>
{
};

struct EnumField : FieldBase<FieldType::Enum>
{
};

using Field = std::variant<PrimitiveField, ArrayField, ClassField, EnumField>;

class FieldDescription
{
public:
    template<typename T, size_t N>
    explicit constexpr FieldDescription(const refl::field_descriptor<T, N>& fieldDesc, const Field& field)
        : mName(fieldDesc.name.c_str())
        , mType(static_cast<FieldType>(field.index()))
        , mField(field)
    {
        // resolve attributes
        std::apply([&](auto attrib)
        {
            mAttributes.push_back({ .description = attrib.description,
                                    .value = std::make_any<decltype(attrib)>(attrib) });
        }, fieldDesc.attributes);
    }
    
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

class ClassDescription
{
public:
    template<typename T>
    explicit constexpr ClassDescription(const refl::type_descriptor<T>& type)
        : mGuid(refl::descriptor::get_attribute<Reflection::Attribute::Guid>(type))
        , mName(type.name.c_str())
    {
        // resolve fields
        size_t fieldOffset = 0;
        auto fields = refl::util::filter(type.members, [](auto member) { return refl::descriptor::is_field(member); });
        refl::util::for_each(fields, [&](auto member)
        {
            using ValueType = typename decltype(member)::value_type;
            
            size_t fieldSize = sizeof(ValueType);
            if constexpr (std::is_fundamental<ValueType>::value)
            {
                auto field = PrimitiveField();
                field.offset = fieldOffset;
                field.size = fieldSize;
                mFields.emplace_back(member, field);
            }
            else if (std::is_array<ValueType>::value)
            {
                auto field = ArrayField();
                field.offset = fieldOffset;
                field.size = fieldSize;
                mFields.emplace_back(member, field);
            }
            else if (std::is_class<ValueType>::value)
            {
                auto field = ClassField();
                field.offset = fieldOffset;
                field.size = fieldSize;
                mFields.emplace_back(member, field);
            }
            else if (std::is_enum<ValueType>::value)
            {
                auto field = EnumField();
                field.offset = fieldOffset;
                field.size = fieldSize;
                mFields.emplace_back(member, field);
            }
            fieldOffset += fieldSize;
        });
        
        // resolve base classes
        refl::util::for_each(type.bases, [&](auto base)
        {
            mBaseClasses.emplace_back(base);
        });
        
        // resolve attributes
        std::apply([&](auto attrib)
        {
            mAttributes.push_back({ .description = attrib.description,
                                    .value = std::make_any<decltype(attrib)>(attrib) });
        }, type.attributes);
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
    TArray<FieldDescription> mFields;
    TArray<ClassDescription> mBaseClasses;
    TArray<AttributePair> mAttributes;
};

} // namespace Gleam::Reflection
