#include "gpch.h"
#include "JSONSerializer.h"

#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>

using namespace Gleam;

namespace rapidjson {

static TString Convert(const rapidjson::Value& value)
{
    StringBuffer buffer;
    PrettyWriter<StringBuffer> writer(buffer);
    value.Accept(writer);
    return buffer.GetString();
}

static Value StringRef(const Guid& guid)
{
    return Value(StringRef(guid.ToString().c_str()));
}

} // namespace rapidjson

static void SerializePrimitiveObject(const void* obj,
                                     Reflection::PrimitiveType type,
                                     rapidjson::Document::AllocatorType& allocator,
                                     rapidjson::Value& node)
{
    switch (type)
    {
        case Reflection::PrimitiveType::Bool:
            node.AddMember("Value", *(bool*)obj, allocator);
            break;
        case Reflection::PrimitiveType::Int8:
            node.AddMember("Value", *(int8_t*)obj, allocator);
            break;
        case Reflection::PrimitiveType::Char:
            node.AddMember("Value", *(char*)obj, allocator);
            break;
        case Reflection::PrimitiveType::Int16:
            node.AddMember("Value", *(int16_t*)obj, allocator);
            break;
        case Reflection::PrimitiveType::Int32:
            node.AddMember("Value", *(int32_t*)obj, allocator);
            break;
        case Reflection::PrimitiveType::Int64:
            node.AddMember("Value", *(int64_t*)obj, allocator);
            break;
        case Reflection::PrimitiveType::UInt8:
            node.AddMember("Value", *(uint8_t*)obj, allocator);
            break;
        case Reflection::PrimitiveType::UInt16:
            node.AddMember("Value", *(uint16_t*)obj, allocator);
            break;
        case Reflection::PrimitiveType::UInt32:
            node.AddMember("Value", *(uint32_t*)obj, allocator);
            break;
        case Reflection::PrimitiveType::UInt64:
            node.AddMember("Value", *(uint64_t*)obj, allocator);
            break;
        case Reflection::PrimitiveType::Float:
            node.AddMember("Value", *(float*)obj, allocator);
            break;
        case Reflection::PrimitiveType::Double:
            node.AddMember("Value", *(double*)obj, allocator);
            break;
        default:
            GLEAM_ASSERT(false, "JSONSerializer: Unknown primitive type");
            break;
    }
}

static void SerializeClassHeader(const Reflection::ClassDescription& classDesc,
                                 const Guid& fieldGuid,
                                 const TStringView fieldName,
                                 rapidjson::Document::AllocatorType& allocator,
                                 rapidjson::Value& outObject)
{
    outObject.AddMember("Kind", rapidjson::StringRef("Class"), allocator);
    outObject.AddMember("FieldGuid", rapidjson::StringRef(fieldGuid), allocator);
    outObject.AddMember("TypeGuid", rapidjson::StringRef(classDesc.Guid()), allocator);
    outObject.AddMember("TypeName", rapidjson::StringRef(classDesc.ResolveName().data()), allocator);
    
    if (not fieldName.empty())
    {
        outObject.AddMember("FieldName", rapidjson::StringRef(fieldName.data()), allocator);
    }
    
    if (classDesc.HasAttribute<Reflection::Attribute::Version>())
    {
        const auto& attr = classDesc.GetAttribute<Reflection::Attribute::Version>();
        const auto& desc = Reflection::Attribute::Version::description;
        outObject.AddMember(rapidjson::StringRef(desc.tag), rapidjson::Value().SetUint(attr.version), allocator);
    }
}

static void SerializeClassObjectFields(const void* obj,
                                       const Guid& fieldGuid,
                                       const TStringView fieldName,
                                       const Reflection::ClassDescription& classDesc,
                                       rapidjson::Document::AllocatorType& allocator,
                                       rapidjson::Value& outFields)
{
    for (const auto& baseClass : classDesc.ResolveBaseClasses())
    {
        SerializeClassObjectFields(obj, fieldGuid, fieldName, baseClass, allocator, outFields);
    }
    
    for (const auto& field : classDesc.ResolveFields())
    {
        auto newGuid = Guid::Combine(fieldGuid, classDesc.Guid());
        if (field.HasAttribute<Reflection::Attribute::Serializable>())
        {
            switch (field.GetType())
            {
                case Reflection::FieldType::Class:
                {
                    auto classField = field.GetField<Reflection::ClassField>();
                    break;
                }
                case Reflection::FieldType::Array:
                {
                    auto arrayField = field.GetField<Reflection::ArrayField>();
                    break;
                }
                case Reflection::FieldType::Enum:
                {
                    auto enumField = field.GetField<Reflection::EnumField>();
                    break;
                }
                case Reflection::FieldType::Primitive:
                {
                    auto primitiveField = field.GetField<Reflection::PrimitiveField>();
                    break;
                }
                default:
                    continue;
            }
        }
    }
}

static void SerializeClassObject(const void* obj,
                                 const Guid& fieldGuid,
                                 const TStringView fieldName,
                                 const Reflection::ClassDescription& classDesc,
                                 rapidjson::Document::AllocatorType& allocator,
                                 rapidjson::Value& outObjects)
{
    rapidjson::Value fields(rapidjson::kArrayType);
    SerializeClassObjectFields(obj, fieldGuid, fieldName, classDesc, allocator, fields);
    if (fields.Size() > 0)
    {
        rapidjson::Value object(rapidjson::kObjectType);
        SerializeClassHeader(classDesc, fieldGuid, fieldName, allocator, object);
        object.AddMember("Fields", fields, allocator);
        outObjects.AddMember("Data", object, allocator);
    }
}

TString JSONSerializer::Serialize(const void* obj, const Reflection::ClassDescription& classDesc)
{
    rapidjson::Document document(rapidjson::kObjectType);
    SerializeClassObject(obj, Guid::InvalidGuid(), "", classDesc, document.GetAllocator(), document);
    return rapidjson::Convert(document);
}
