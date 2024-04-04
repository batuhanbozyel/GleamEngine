#include "gpch.h"
#include "JSONSerializer.h"

#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>

using namespace Gleam;

namespace rapidjson {

struct Node
{
    Document::AllocatorType& allocator;
    Value& object;
    
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
    
    explicit Node(Value& object, Document::AllocatorType& allocator)
        : object(object), allocator(allocator)
    {
        
    }
};

static TString Convert(const rapidjson::Value& value)
{
    StringBuffer buffer;
    PrettyWriter<StringBuffer> writer(buffer);
    value.Accept(writer);
    return buffer.GetString();
}

static Value::StringRefType StringRef(const Guid& guid)
{
    return StringRef(guid.ToString().c_str());
}

} // namespace rapidjson

static void SerializePrimitiveObjectValue(const void* obj,
                                          Reflection::PrimitiveType type,
                                          rapidjson::Node& node);

static void SerializeClassObjectFields(const void* obj,
                                       const Guid& fieldGuid,
                                       const TStringView fieldName,
                                       const Reflection::ClassDescription& classDesc,
                                       rapidjson::Node& node);

static void SerializePrimitiveObject(const void* obj,
                                     const Guid& fieldGuid,
                                     const TStringView fieldName,
                                     Reflection::PrimitiveType type,
                                     rapidjson::Node& node);

static void SerializeClassObject(const void* obj,
                                 const Guid& fieldGuid,
                                 const TStringView fieldName,
                                 const Reflection::ClassDescription& classDesc,
                                 rapidjson::Node& node);

static void SerializePrimitiveHeader(Reflection::PrimitiveType type,
                                     const Guid& fieldGuid,
                                     const TStringView fieldName,
                                     rapidjson::Node& node);

static void SerializeClassHeader(const Reflection::ClassDescription& classDesc,
                                 const Guid& fieldGuid,
                                 const TStringView fieldName,
                                 rapidjson::Node& node);

void JSONSerializer::Initialize()
{
    mCustomSerializers[typeid(TString).hash_code()] = [](const void* obj,
                                                         const Guid& fieldGuid,
                                                         const TStringView fieldName,
                                                         const Reflection::ClassDescription& classDesc,
                                                         void* userData)
    {
        auto str = Reflection::Get<TString>(obj).c_str();
        auto& outObject = Reflection::Get<rapidjson::Node>(userData);
        rapidjson::Value object(rapidjson::kObjectType);
        rapidjson::Node node(object, outObject.allocator);
        node.AddMember("Value", rapidjson::StringRef(str));
        SerializeClassHeader(classDesc, fieldGuid, fieldName, outObject);
        outObject.AddMember("Data", object);
    };
}

void JSONSerializer::Shutdown()
{
    mCustomSerializers.clear();
}

void SerializePrimitiveHeader(Reflection::PrimitiveType type,
                              const Guid& fieldGuid,
                              const TStringView fieldName,
                              rapidjson::Node& node)
{
    node.AddMember("Kind", rapidjson::StringRef("Primitive"));
    node.AddMember("FieldGuid", rapidjson::StringRef(fieldGuid));
    node.AddMember("TypeName", rapidjson::StringRef(Reflection::Database::GetPrimitiveName(type).data()));
    
    if (not fieldName.empty())
    {
        node.AddMember("FieldName", rapidjson::StringRef(fieldName.data()));
    }
}

void SerializeClassHeader(const Reflection::ClassDescription& classDesc,
                          const Guid& fieldGuid,
                          const TStringView fieldName,
                          rapidjson::Node& node)
{
    node.AddMember("Kind", rapidjson::StringRef("Class"));
    node.AddMember("FieldGuid", rapidjson::StringRef(fieldGuid));
    node.AddMember("TypeGuid", rapidjson::StringRef(classDesc.Guid()));
    node.AddMember("TypeName", rapidjson::StringRef(classDesc.ResolveName().data()));
    
    if (not fieldName.empty())
    {
        node.AddMember("FieldName", rapidjson::StringRef(fieldName.data()));
    }
    
    if (classDesc.HasAttribute<Reflection::Attribute::Version>())
    {
        const auto& attr = classDesc.GetAttribute<Reflection::Attribute::Version>();
        const auto& desc = Reflection::Attribute::Version::description;
        node.AddMember(rapidjson::StringRef(desc.tag), rapidjson::Value().SetUint(attr.version));
    }
}

void SerializePrimitiveObjectValue(const void* obj,
                                   Reflection::PrimitiveType type,
                                   rapidjson::Node& node)
{
    switch (type)
    {
        case Reflection::PrimitiveType::Bool:
            node.AddMember("Value", Reflection::Get<bool>(obj));
            break;
        case Reflection::PrimitiveType::Int8:
            node.AddMember("Value", Reflection::Get<int8_t>(obj));
            break;
        case Reflection::PrimitiveType::WChar:
            node.AddMember("Value", Reflection::Get<wchar_t>(obj));
            break;
        case Reflection::PrimitiveType::Char:
            node.AddMember("Value", Reflection::Get<char>(obj));
            break;
        case Reflection::PrimitiveType::Int16:
            node.AddMember("Value", Reflection::Get<int16_t>(obj));
            break;
        case Reflection::PrimitiveType::Int32:
            node.AddMember("Value", Reflection::Get<int32_t>(obj));
            break;
        case Reflection::PrimitiveType::Int64:
            node.AddMember("Value", Reflection::Get<int64_t>(obj));
            break;
        case Reflection::PrimitiveType::UInt8:
            node.AddMember("Value", Reflection::Get<uint8_t>(obj));
            break;
        case Reflection::PrimitiveType::UInt16:
            node.AddMember("Value", Reflection::Get<uint16_t>(obj));
            break;
        case Reflection::PrimitiveType::UInt32:
            node.AddMember("Value", Reflection::Get<uint32_t>(obj));
            break;
        case Reflection::PrimitiveType::UInt64:
            node.AddMember("Value", Reflection::Get<uint64_t>(obj));
            break;
        case Reflection::PrimitiveType::Float:
            node.AddMember("Value", Reflection::Get<float>(obj));
            break;
        case Reflection::PrimitiveType::Double:
            node.AddMember("Value", Reflection::Get<double>(obj));
            break;
        default:
            GLEAM_ASSERT(false, "JSONSerializer: Unknown primitive type");
            break;
    }
}

void SerializeClassObjectFields(const void* obj,
                                const Guid& fieldGuid,
                                const TStringView fieldName,
                                const Reflection::ClassDescription& classDesc,
                                rapidjson::Node& outFields)
{
    for (const auto& baseClass : classDesc.ResolveBaseClasses())
    {
        SerializeClassObjectFields(obj, fieldGuid, fieldName, baseClass, outFields);
    }
    
    for (const auto& field : classDesc.ResolveFields())
    {
        if (field.HasAttribute<Reflection::Attribute::Serializable>())
        {
            auto newGuid = Guid::Combine(fieldGuid, classDesc.Guid());
            auto fieldObject = rapidjson::Value(rapidjson::kObjectType);
            auto fieldNode = rapidjson::Node(fieldObject, outFields.allocator);
            switch (field.GetType())
            {
                case Reflection::FieldType::Class:
                {
                    auto classField = field.GetField<Reflection::ClassField>();
                    if (JSONSerializer::TryCustomSerializer(obj, newGuid, field.ResolveName(), classField, &fieldNode) == false)
                    {
                        const auto& fieldDesc = Reflection::GetClass(classField.hash);
                        SerializeClassObject(OffsetPointer(obj, classField.offset), newGuid, field.ResolveName(), fieldDesc, fieldNode);
                    }
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
                    auto primitiveType = primitiveField.primitive;
                    SerializePrimitiveObject(OffsetPointer(obj, primitiveField.offset), newGuid, field.ResolveName(), primitiveType, fieldNode);
                    break;
                }
                default:
                    continue;
            }
            outFields.PushBack(fieldNode.object);
        }
    }
}

void SerializePrimitiveObject(const void* obj,
                              const Guid& fieldGuid,
                              const TStringView fieldName,
                              Reflection::PrimitiveType type,
                              rapidjson::Node& outObject)
{
    rapidjson::Value object(rapidjson::kObjectType);
    rapidjson::Node node(object, outObject.allocator);
    SerializePrimitiveObjectValue(obj, type, node);
    SerializePrimitiveHeader(type, fieldGuid, fieldName, outObject);
    outObject.AddMember("Data", object);
}

void SerializeClassObject(const void* obj,
                          const Guid& fieldGuid,
                          const TStringView fieldName,
                          const Reflection::ClassDescription& classDesc,
                          rapidjson::Node& outObjects)
{
    rapidjson::Value fields(rapidjson::kArrayType);
    rapidjson::Node fieldsNode(fields, outObjects.allocator);
    SerializeClassObjectFields(obj, fieldGuid, fieldName, classDesc, fieldsNode);
    if (fields.Size() > 0)
    {
        rapidjson::Value object(rapidjson::kObjectType);
        rapidjson::Node objectNode(object, outObjects.allocator);
        SerializeClassHeader(classDesc, fieldGuid, fieldName, objectNode);
        objectNode.AddMember("Fields", fields);
        outObjects.AddMember("Data", object);
    }
}

TString JSONSerializer::Serialize(const void* obj, const Reflection::ClassDescription& classDesc)
{
    rapidjson::Document document(rapidjson::kObjectType);
    rapidjson::Node root(document, document.GetAllocator());
    SerializeClassObject(obj, Guid::InvalidGuid(), "", classDesc, root);
    return rapidjson::Convert(document);
}

bool JSONSerializer::TryCustomSerializer(const void* obj,
                                         const Guid& fieldGuid,
                                         const TStringView fieldName,
                                         const Reflection::ClassField& classField,
                                         void* userData)
{
    auto it = mCustomSerializers.find(classField.hash);
    if (it != mCustomSerializers.end())
    {
        const auto& fieldDesc = Reflection::GetClass(classField.hash);
        it->second(obj, fieldGuid, fieldName, fieldDesc, userData);
        return true;
    }
    return false;
}
