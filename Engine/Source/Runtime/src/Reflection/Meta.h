#pragma once
#include "Attribute.h"

namespace Gleam::Reflection {

template<typename T>
concept AttributeType = std::is_base_of_v<refl::attr::usage::type, T> 
                     || std::is_base_of_v<refl::attr::usage::field, T>;

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

class PrimitiveDescription
{
public:
    explicit PrimitiveDescription()
    {
        
    }
private:
};

class FieldDescription
{
public:
    explicit FieldDescription()
    {
        
    }
    
    const TStringView ResolveName() const
    {
        return mName;
    }
    
    template<AttributeType T>
    bool HasAttribute() const
    {
        
    }
    
    template<AttributeType T>
    T GetAttribute() const
    {
        
    }
    
private:
    
    Guid guid;
    FieldType mType;
    TStringView mName;
};

template<typename T>
class ClassDescription
{
public:
    explicit constexpr ClassDescription(const refl::type_descriptor<T>& type)
        : mType(type)
    {
        mGuid = GetAttribute<Attribute::Guid>();
    }
    
    const Gleam::Guid& Guid() const
    {
        return mGuid;
    }
    
    const TStringView ResolveName() const
    {
        return mType.name;
    }
    
    const TArray<FieldDescription>& ResolveFields() const
    {
        
    }
    
    const TArray<ClassDescription>& ResolveBaseClasses() const
    {
        
    }
    
    template<AttributeType Attrib>
    constexpr bool HasAttribute() const noexcept
    {
        return refl::descriptor::has_attribute<Attrib>(mType);
    }
    
    template<AttributeType Attrib>
    constexpr const Attrib& GetAttribute() const noexcept
    {
        return refl::descriptor::get_attribute<Attrib>(mType);
    }
    
private:
    
    Gleam::Guid mGuid;
    TArray<FieldDescription> mFields;
    TArray<ClassDescription> mBaseClasses;
    refl::type_descriptor<T> mType;
};

} // namespace Gleam::Reflection
