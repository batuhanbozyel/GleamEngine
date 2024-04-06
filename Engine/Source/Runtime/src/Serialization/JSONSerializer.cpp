#include "gpch.h"
#include "JSONSerializer.h"

#define RAPIDJSON_HAS_STDSTRING 1
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

} // namespace rapidjson

/*
 Serialize headers
 */
static void SerializePrimitiveHeader(Reflection::PrimitiveType type,
                                     const Guid& fieldGuid,
                                     const TStringView fieldName,
                                     rapidjson::Node& node);

static void SerializeEnumHeader(const Reflection::EnumDescription& enumDesc,
                                const Guid& fieldGuid,
                                const TStringView fieldName,
                                rapidjson::Node& node);

static void SerializeClassHeader(const Reflection::ClassDescription& classDesc,
                                 const Guid& fieldGuid,
                                 const TStringView fieldName,
                                 rapidjson::Node& node);

/*
 Serialize values
 */
static void SerializePrimitiveObjectValue(const void* obj,
                                          Reflection::PrimitiveType type,
                                          rapidjson::Node& node);

static void SerializeEnumObjectValue(const void* obj,
                                     rapidjson::Node& node);


static void SerializeClassObjectFields(const void* obj,
                                       const Guid& fieldGuid,
                                       const TStringView fieldName,
                                       const Reflection::ClassDescription& classDesc,
                                       rapidjson::Node& node);

/*
 Serialize objects
 */
static void SerializePrimitiveObject(const void* obj,
                                     const Guid& fieldGuid,
                                     const TStringView fieldName,
                                     Reflection::PrimitiveType type,
                                     rapidjson::Node& node);

static void SerializeEnumObject(const void* obj,
                                const Guid& fieldGuid,
                                const TStringView fieldName,
                                const Reflection::EnumDescription& enumDesc,
                                rapidjson::Node& outObject);

static void SerializeClassObject(const void* obj,
                                 const Guid& fieldGuid,
                                 const TStringView fieldName,
                                 const Reflection::ClassDescription& classDesc,
                                 rapidjson::Node& node);

static void SerializeArrayObject(const void* obj,
                                 const Guid& fieldGuid,
                                 const TStringView fieldName,
                                 const Reflection::ArrayField& arrayField,
                                 rapidjson::Node& outObjects);

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

/*
 Serialize headers
 */
void SerializePrimitiveHeader(Reflection::PrimitiveType type,
                              const Guid& fieldGuid,
                              const TStringView fieldName,
                              rapidjson::Node& node)
{
    node.AddMember("Kind", rapidjson::StringRef("Primitive"));
    node.AddMember("FieldGuid", fieldGuid.ToString());
    node.AddMember("TypeName", rapidjson::StringRef(Reflection::Database::GetPrimitiveName(type).data()));
    
    if (not fieldName.empty())
    {
        node.AddMember("FieldName", rapidjson::StringRef(fieldName.data()));
    }
}

void SerializeEnumHeader(const Reflection::EnumDescription& enumDesc,
                         const Guid& fieldGuid,
                         const TStringView fieldName,
                         rapidjson::Node& node)
{
    node.AddMember("Kind", rapidjson::StringRef("Enum"));
    node.AddMember("FieldGuid", fieldGuid.ToString());
    node.AddMember("TypeGuid", enumDesc.Guid().ToString());
    node.AddMember("TypeName", rapidjson::StringRef(enumDesc.ResolveName().data()));
    
    if (not fieldName.empty())
    {
        node.AddMember("FieldName", rapidjson::StringRef(fieldName.data()));
    }
    
    if (enumDesc.HasAttribute<Reflection::Attribute::Version>())
    {
        const auto& attr = enumDesc.GetAttribute<Reflection::Attribute::Version>();
        const auto& desc = Reflection::Attribute::Version::description;
        node.AddMember(rapidjson::StringRef(desc.tag), rapidjson::Value().SetUint(attr.version));
    }
}

void SerializeClassHeader(const Reflection::ClassDescription& classDesc,
                          const Guid& fieldGuid,
                          const TStringView fieldName,
                          rapidjson::Node& node)
{
    node.AddMember("Kind", rapidjson::StringRef("Class"));
    node.AddMember("FieldGuid", fieldGuid.ToString());
    node.AddMember("TypeGuid", classDesc.Guid().ToString());
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

/*
 Serialize values
 */
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

void SerializeEnumObjectValue(const void* obj,
                              rapidjson::Node& node)
{
    node.AddMember("Value", Reflection::Get<int>(obj));
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
                    SerializeArrayObject(OffsetPointer(obj, arrayField.offset), newGuid, field.ResolveName(), arrayField, fieldNode);
                    break;
                }
                case Reflection::FieldType::Enum:
                {
                    auto enumField = field.GetField<Reflection::EnumField>();
                    const auto& enumDesc = Reflection::GetEnum(enumField.hash);
                    SerializeEnumObject(OffsetPointer(obj, enumField.offset), newGuid, field.ResolveName(), enumDesc, fieldNode);
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

/*
 Serialize objects
 */
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

void SerializeEnumObject(const void* obj,
                         const Guid& fieldGuid,
                         const TStringView fieldName,
                         const Reflection::EnumDescription& enumDesc,
                         rapidjson::Node& outObject)
{
    rapidjson::Value object(rapidjson::kObjectType);
    rapidjson::Node node(object, outObject.allocator);
    SerializeEnumObjectValue(obj, node);
    SerializeEnumHeader(enumDesc, fieldGuid, fieldName, outObject);
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

void SerializeArrayObject(const void* obj,
                          const Guid& fieldGuid,
                          const TStringView fieldName,
                          const Reflection::ArrayField& arrayField,
                          rapidjson::Node& outObjects)
{
    rapidjson::Value header(rapidjson::kObjectType);
    rapidjson::Node headerNode(header, outObjects.allocator);
    
    rapidjson::Value elements(rapidjson::kArrayType);
    rapidjson::Node elementsNode(elements, outObjects.allocator);
    if (arrayField.elementType == Reflection::FieldType::Primitive)
    {
        auto primitiveType = Reflection::Database::GetPrimitiveType(arrayField.elementHash);
        headerNode.AddMember("Kind", rapidjson::StringRef("Primitive"));
        
        for (size_t elementOffset = 0; elementOffset < arrayField.size; elementOffset += arrayField.stride)
        {
            SerializePrimitiveObjectValue(OffsetPointer(obj, elementOffset), primitiveType, elementsNode);
        }
    }
    else if (arrayField.elementType == Reflection::FieldType::Array)
    {
        const auto& arrayDesc = Reflection::GetClass(arrayField.elementHash);
        auto innerField = Reflection::ArrayField(arrayDesc.ResolveName(), Reflection::FieldType::Array, arrayField.elementHash, arrayDesc.GetSize());
        
        headerNode.AddMember("Kind", rapidjson::StringRef("Array"));
        headerNode.AddMember("TypeGuid", arrayDesc.Guid().ToString());
        
        if (arrayDesc.HasAttribute<Reflection::Attribute::Version>())
        {
            const auto& attr = arrayDesc.GetAttribute<Reflection::Attribute::Version>();
            const auto& desc = Reflection::Attribute::Version::description;
            headerNode.AddMember(rapidjson::StringRef(desc.tag), rapidjson::Value().SetUint(attr.version));
        }
        
        rapidjson::Value innerElements(rapidjson::kArrayType);
        rapidjson::Node innerElementsNode(innerElements, outObjects.allocator);
        for (size_t elementOffset = 0; elementOffset < arrayField.size; elementOffset += arrayField.stride)
        {
            SerializeArrayObject(OffsetPointer(obj, elementOffset), fieldGuid, arrayField.elementName, innerField, innerElementsNode);
        }
    }
    else if (arrayField.elementType == Reflection::FieldType::Class)
    {
        auto classDesc = Reflection::Database::GetClass(arrayField.elementHash);
        headerNode.AddMember("Kind", rapidjson::StringRef("Class"));
        headerNode.AddMember("TypeGuid", classDesc.Guid().ToString());
        
        if (classDesc.HasAttribute<Reflection::Attribute::Version>())
        {
            const auto& attr = classDesc.GetAttribute<Reflection::Attribute::Version>();
            const auto& desc = Reflection::Attribute::Version::description;
            headerNode.AddMember(rapidjson::StringRef(desc.tag), rapidjson::Value().SetUint(attr.version));
        }
        
        for (size_t elementOffset = 0; elementOffset < arrayField.size; elementOffset += arrayField.stride)
        {
            SerializeClassObjectFields(OffsetPointer(obj, elementOffset), fieldGuid, fieldName, classDesc, elementsNode);
        }
    }
    else if (arrayField.elementType == Reflection::FieldType::Enum)
    {
        auto enumDesc = Reflection::Database::GetEnum(arrayField.elementHash);
        headerNode.AddMember("Kind", rapidjson::StringRef("Enum"));
        headerNode.AddMember("TypeGuid", enumDesc.Guid().ToString());
        
        if (enumDesc.HasAttribute<Reflection::Attribute::Version>())
        {
            const auto& attr = enumDesc.GetAttribute<Reflection::Attribute::Version>();
            const auto& desc = Reflection::Attribute::Version::description;
            headerNode.AddMember(rapidjson::StringRef(desc.tag), rapidjson::Value().SetUint(attr.version));
        }
        
        for (size_t elementOffset = 0; elementOffset < arrayField.size; elementOffset += arrayField.stride)
        {
            SerializeEnumObjectValue(OffsetPointer(obj, elementOffset), elementsNode);
        }
    }
    
    // Header
    if (elements.Size() > 0)
    {
        headerNode.AddMember("TypeName", rapidjson::StringRef(arrayField.elementName.data()));
        headerNode.AddMember("FieldGuid", fieldGuid.ToString());
        if (not fieldName.empty())
        {
            headerNode.AddMember("FieldName", rapidjson::StringRef(fieldName.data()));
        }
        headerNode.AddMember("Elements", elements);
        outObjects.AddMember("Data", header);
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
