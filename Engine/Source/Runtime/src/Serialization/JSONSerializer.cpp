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
    writer.SetFormatOptions(PrettyFormatOptions::kFormatSingleLineArray);
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

static void SerializeArrayHeader(const Reflection::ArrayDescription& arrayDesc,
                                 const Guid& fieldGuid,
                                 const TStringView fieldName,
                                 rapidjson::Node& node);

/*
 Serialize values
 */
static void SerializePrimitiveObjectValue(const void* obj,
                                          Reflection::PrimitiveType type,
                                          rapidjson::Node& node);

static void SerializePrimitiveArrayValue(const void* obj,
                                         Reflection::PrimitiveType type,
                                         rapidjson::Node& node);

static void SerializeEnumObjectValue(const void* obj,
                                     rapidjson::Node& node);

static void SerializeEnumArrayValue(const void* obj,
                                    rapidjson::Node& node);

static void SerializeClassObjectFields(const void* obj,
                                       const Guid& fieldGuid,
                                       const TStringView fieldName,
                                       const Reflection::ClassDescription& classDesc,
                                       rapidjson::Node& node);

static void SerializeClassArrayFields(const void* obj,
                                      const Reflection::ClassDescription& classDesc,
                                      rapidjson::Node& node);

static void SerializeArrayObjectElements(const void* obj,
                                         const Reflection::ArrayDescription& arrayDesc,
                                         rapidjson::Node& outElements);

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
                                 const Reflection::ArrayDescription& arrayDesc,
                                 rapidjson::Node& outObjects);

void JSONSerializer::Initialize()
{
    mCustomObjectSerializers[Reflection::GetClass<TString>().ResolveName()] = [](const void* obj,
                                                                           const Guid& fieldGuid,
                                                                           const TStringView fieldName,
                                                                           const Reflection::ClassDescription& classDesc,
                                                                           void* userData)
    {
        const auto& str = Reflection::Get<TString>(obj);
        auto& outObject = Reflection::Get<rapidjson::Node>(userData);
        rapidjson::Value object(rapidjson::kObjectType);
        rapidjson::Node node(object, outObject.allocator);
        node.AddMember("Value", rapidjson::StringRef(str.c_str(), str.length()));
        SerializeClassHeader(classDesc, fieldGuid, fieldName, outObject);
        outObject.AddMember("Data", object);
    };
    
    mCustomArraySerializers[Reflection::GetClass<TString>().ResolveName()] = [](const void* obj,
                                                                                const Reflection::ClassDescription& classDesc,
                                                                                void* userData)
    {
        const auto& str = Reflection::Get<TString>(obj);
        auto& outObject = Reflection::Get<rapidjson::Node>(userData);
        outObject.PushBack(rapidjson::Value(rapidjson::StringRef(str.c_str(), str.length())));
    };
    
    mCustomObjectSerializers[Reflection::GetClass<TArray<intptr_t>>().ResolveName()] = [](const void* obj,
																						  const Guid& fieldGuid,
																						  const TStringView fieldName,
																						  const Reflection::ClassDescription& classDesc,
																						  void* userData)
    {
        const auto& arr = Reflection::Get<TArray<intptr_t>>(obj);
        auto& outObject = Reflection::Get<rapidjson::Node>(userData);
        const auto& arrDesc = Reflection::GetArray(classDesc.ContainerHash());
        auto containerDesc = Reflection::ArrayDescription(arrDesc.ResolveName(), arrDesc.ElementType(), arrDesc.ElementHash(), arrDesc.GetStride() * arr.size(), arrDesc.GetStride());
        SerializeArrayObject(arr.data(), fieldGuid, fieldName, containerDesc, outObject);
    };
    
    mCustomArraySerializers[Reflection::GetClass<TArray<intptr_t>>().ResolveName()] = [](const void* obj,
                                                                                         const Reflection::ClassDescription& classDesc,
                                                                                         void* userData)
    {
        const auto& arr = Reflection::Get<TArray<intptr_t>>(obj);
        auto& outObject = Reflection::Get<rapidjson::Node>(userData);
        const auto& arrDesc = Reflection::GetArray(classDesc.ContainerHash());
        auto containerDesc = Reflection::ArrayDescription(arrDesc.ResolveName(), arrDesc.ElementType(), arrDesc.ElementHash(), arrDesc.GetStride() * arr.size(), arrDesc.GetStride());
        SerializeArrayObjectElements(arr.data(), containerDesc, outObject);
    };
}

void JSONSerializer::Shutdown()
{
    mCustomObjectSerializers.clear();
    mCustomArraySerializers.clear();
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

void SerializeArrayHeader(const Reflection::ArrayDescription& arrayDesc,
                          const Guid& fieldGuid,
                          const TStringView fieldName,
                          rapidjson::Node& node)
{
    if (arrayDesc.ElementType() == Reflection::FieldType::Primitive)
    {
        node.AddMember("Kind", rapidjson::StringRef("Primitive"));
    }
    else if (arrayDesc.ElementType() == Reflection::FieldType::Array)
    {
        node.AddMember("Kind", rapidjson::StringRef("Array"));
    }
    else if (arrayDesc.ElementType() == Reflection::FieldType::Class)
    {
		const auto& classDesc = Reflection::Database::GetClass(arrayDesc.ElementHash());
        node.AddMember("Kind", rapidjson::StringRef("Class"));
        node.AddMember("TypeGuid", classDesc.Guid().ToString());
    }
    else if (arrayDesc.ElementType() == Reflection::FieldType::Enum)
    {
		const auto& enumDesc = Reflection::Database::GetEnum(arrayDesc.ElementHash());
        node.AddMember("Kind", rapidjson::StringRef("Enum"));
        node.AddMember("TypeGuid", enumDesc.Guid().ToString());
        
        if (enumDesc.HasAttribute<Reflection::Attribute::Version>())
        {
            const auto& attr = enumDesc.GetAttribute<Reflection::Attribute::Version>();
            const auto& desc = Reflection::Attribute::Version::description;
            node.AddMember(rapidjson::StringRef(desc.tag), rapidjson::Value().SetUint(attr.version));
        }
    }
    node.AddMember("TypeName", rapidjson::StringRef(arrayDesc.ResolveName().data()));
    node.AddMember("FieldGuid", fieldGuid.ToString());
    if (not fieldName.empty())
    {
        node.AddMember("FieldName", rapidjson::StringRef(fieldName.data()));
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

void SerializePrimitiveArrayValue(const void* obj,
                                  Reflection::PrimitiveType type,
                                  rapidjson::Node& node)
{
    switch (type)
    {
        case Reflection::PrimitiveType::Bool:
            node.PushBack(rapidjson::Value(Reflection::Get<bool>(obj)));
            break;
        case Reflection::PrimitiveType::Int8:
            node.PushBack(rapidjson::Value(Reflection::Get<int8_t>(obj)));
            break;
        case Reflection::PrimitiveType::WChar:
            node.PushBack(rapidjson::Value(Reflection::Get<wchar_t>(obj)));
            break;
        case Reflection::PrimitiveType::Char:
            node.PushBack(rapidjson::Value(Reflection::Get<char>(obj)));
            break;
        case Reflection::PrimitiveType::Int16:
            node.PushBack(rapidjson::Value(Reflection::Get<int16_t>(obj)));
            break;
        case Reflection::PrimitiveType::Int32:
            node.PushBack(rapidjson::Value(Reflection::Get<int32_t>(obj)));
            break;
        case Reflection::PrimitiveType::Int64:
            node.PushBack(rapidjson::Value(Reflection::Get<int64_t>(obj)));
            break;
        case Reflection::PrimitiveType::UInt8:
            node.PushBack(rapidjson::Value(Reflection::Get<uint8_t>(obj)));
            break;
        case Reflection::PrimitiveType::UInt16:
            node.PushBack(rapidjson::Value(Reflection::Get<uint16_t>(obj)));
            break;
        case Reflection::PrimitiveType::UInt32:
            node.PushBack(rapidjson::Value(Reflection::Get<uint32_t>(obj)));
            break;
        case Reflection::PrimitiveType::UInt64:
            node.PushBack(rapidjson::Value(Reflection::Get<uint64_t>(obj)));
            break;
        case Reflection::PrimitiveType::Float:
            node.PushBack(rapidjson::Value(Reflection::Get<float>(obj)));
            break;
        case Reflection::PrimitiveType::Double:
            node.PushBack(rapidjson::Value(Reflection::Get<double>(obj)));
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

void SerializeEnumArrayValue(const void* obj,
                             rapidjson::Node& node)
{
    node.PushBack(rapidjson::Value(Reflection::Get<int>(obj)));
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
					const auto& classField = field.GetField<Reflection::ClassField>();
                    if (JSONSerializer::TryCustomObjectSerializer(OffsetPointer(obj, classField.offset), newGuid, field.ResolveName(), classField, &fieldNode) == false)
                    {
                        const auto& fieldDesc = Reflection::GetClass(classField.hash);
                        SerializeClassObject(OffsetPointer(obj, classField.offset), newGuid, field.ResolveName(), fieldDesc, fieldNode);
                    }
                    break;
                }
                case Reflection::FieldType::Array:
                {
					const auto& arrayField = field.GetField<Reflection::ArrayField>();
                    const auto& arrayDesc = Reflection::GetArray(arrayField.hash);
                    SerializeArrayObjectElements(OffsetPointer(obj, arrayField.offset), arrayDesc, fieldNode);
                    break;
                }
                case Reflection::FieldType::Enum:
                {
					const auto& enumField = field.GetField<Reflection::EnumField>();
                    const auto& enumDesc = Reflection::GetEnum(enumField.hash);
                    SerializeEnumObject(OffsetPointer(obj, enumField.offset), newGuid, field.ResolveName(), enumDesc, fieldNode);
                    break;
                }
                case Reflection::FieldType::Primitive:
                {
					const auto& primitiveField = field.GetField<Reflection::PrimitiveField>();
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

void SerializeClassArrayFields(const void* obj,
                               const Reflection::ClassDescription& classDesc,
                               rapidjson::Node& outFields)
{
    for (const auto& baseClass : classDesc.ResolveBaseClasses())
    {
        SerializeClassArrayFields(obj, baseClass, outFields);
    }
    
    for (const auto& field : classDesc.ResolveFields())
    {
        if (field.HasAttribute<Reflection::Attribute::Serializable>())
        {
            switch (field.GetType())
            {
                case Reflection::FieldType::Class:
                {
                    auto elements = rapidjson::Value(rapidjson::kArrayType);
                    auto elementsNode = rapidjson::Node(elements, outFields.allocator);
					const auto& classField = field.GetField<Reflection::ClassField>();
                    if (JSONSerializer::TryCustomArraySerializer(OffsetPointer(obj, classField.offset), classField, &elementsNode) == false)
                    {
                        const auto& fieldDesc = Reflection::GetClass(classField.hash);
                        SerializeClassArrayFields(OffsetPointer(obj, classField.offset), fieldDesc, elementsNode);
                    }
                    outFields.PushBack(elementsNode.object);
                    break;
                }
                case Reflection::FieldType::Array:
                {
                    auto elements = rapidjson::Value(rapidjson::kArrayType);
                    auto elementsNode = rapidjson::Node(elements, outFields.allocator);
                    
					const auto& arrayField = field.GetField<Reflection::ArrayField>();
                    const auto& arrayDesc = Reflection::GetArray(arrayField.hash);
                    SerializeArrayObjectElements(OffsetPointer(obj, arrayField.offset), arrayDesc, elementsNode);
                    outFields.PushBack(elementsNode.object);
                    break;
                }
                case Reflection::FieldType::Enum:
                {
					const auto& enumField = field.GetField<Reflection::EnumField>();
                    SerializeEnumArrayValue(OffsetPointer(obj, enumField.offset), outFields);
                    break;
                }
                case Reflection::FieldType::Primitive:
                {
                    const auto& primitiveField = field.GetField<Reflection::PrimitiveField>();
                    auto primitiveType = primitiveField.primitive;
                    SerializePrimitiveArrayValue(OffsetPointer(obj, primitiveField.offset), primitiveType, outFields);
                    break;
                }
                default:
                    continue;
            }
        }
    }
}


void SerializeArrayObjectElements(const void* obj,
                                  const Reflection::ArrayDescription& arrayDesc,
                                  rapidjson::Node& outElements)
{
    if (arrayDesc.ElementType() == Reflection::FieldType::Primitive)
    {
        auto primitiveType = Reflection::Database::GetPrimitiveType(arrayDesc.ElementHash());
        for (size_t elementOffset = 0; elementOffset < arrayDesc.GetSize(); elementOffset += arrayDesc.GetStride())
        {
            SerializePrimitiveArrayValue(OffsetPointer(obj, elementOffset), primitiveType, outElements);
        }
    }
    else if (arrayDesc.ElementType() == Reflection::FieldType::Array)
    {
        const auto& innerDesc = Reflection::GetArray(arrayDesc.ElementHash());
        for (size_t elementOffset = 0; elementOffset < arrayDesc.GetSize(); elementOffset += arrayDesc.GetStride())
        {
            rapidjson::Value elements(rapidjson::kArrayType);
            rapidjson::Node elementsNode(elements, outElements.allocator);
            SerializeArrayObjectElements(OffsetPointer(obj, elementOffset), innerDesc, elementsNode);
            outElements.PushBack(elementsNode.object);
        }
    }
    else if (arrayDesc.ElementType() == Reflection::FieldType::Class)
    {
        const auto& classDesc = Reflection::Database::GetClass(arrayDesc.ElementHash());
        for (size_t elementOffset = 0; elementOffset < arrayDesc.GetSize(); elementOffset += arrayDesc.GetStride())
        {
            auto elements = rapidjson::Value(rapidjson::kArrayType);
            auto elementsNode = rapidjson::Node(elements, outElements.allocator);
            SerializeClassArrayFields(OffsetPointer(obj, elementOffset), classDesc, elementsNode);
            outElements.PushBack(elementsNode.object);
        }
    }
    else if (arrayDesc.ElementType() == Reflection::FieldType::Enum)
    {
        const auto& enumDesc = Reflection::Database::GetEnum(arrayDesc.ElementHash());
        for (size_t elementOffset = 0; elementOffset < arrayDesc.GetSize(); elementOffset += arrayDesc.GetStride())
        {
            SerializeEnumArrayValue(OffsetPointer(obj, elementOffset), outElements);
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
                          const Reflection::ArrayDescription& arrayDesc,
                          rapidjson::Node& outObjects)
{
    rapidjson::Value elements(rapidjson::kArrayType);
    rapidjson::Node elementsNode(elements, outObjects.allocator);
    SerializeArrayObjectElements(obj, arrayDesc, elementsNode);
    if (elements.Size() > 0)
    {
        rapidjson::Value object(rapidjson::kObjectType);
        rapidjson::Node objectNode(object, outObjects.allocator);
        SerializeArrayHeader(arrayDesc, fieldGuid, fieldName, objectNode);
        objectNode.AddMember("Elements", elements);
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

bool JSONSerializer::TryCustomObjectSerializer(const void* obj,
                                               const Guid& fieldGuid,
                                               const TStringView fieldName,
                                               const Reflection::ClassField& classField,
                                               void* userData)
{
    const auto& fieldDesc = Reflection::GetClass(classField.hash);
    auto it = mCustomObjectSerializers.find(fieldDesc.ResolveName());
    if (it != mCustomObjectSerializers.end())
    {
        it->second(obj, fieldGuid, fieldName, fieldDesc, userData);
        return true;
    }
    return false;
}

bool JSONSerializer::TryCustomArraySerializer(const void* obj,
                                              const Reflection::ClassField& classField,
                                              void* userData)
{
    const auto& fieldDesc = Reflection::GetClass(classField.hash);
    auto it = mCustomArraySerializers.find(fieldDesc.ResolveName());
    if (it != mCustomArraySerializers.end())
    {
        it->second(obj, fieldDesc, userData);
        return true;
    }
    return false;
}
