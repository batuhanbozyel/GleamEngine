#include "gpch.h"
#include "JSONSerializer.h"
#include "JSONInternal.h"

using namespace Gleam;

struct JSONSerializer::Impl
{
	FileStream& stream;
    
    Impl(FileStream& stream)
		: stream(stream)
    {
        
    }
    
#pragma region mark SerializeForwardDecl

#pragma region mark SerializeHeaders
    static void SerializePrimitiveHeader(Reflection::PrimitiveType type,
                                         const TStringView fieldName,
                                         rapidjson::Node& node);

    static void SerializeEnumHeader(const Reflection::EnumDescription& enumDesc,
                                    const TStringView fieldName,
                                    rapidjson::Node& node);
    
    static void SerializeClassHeader(const Reflection::ClassDescription& classDesc,
                                     const TStringView fieldName,
                                     rapidjson::Node& node);

    static void SerializeArrayHeader(const Reflection::ArrayDescription& arrayDesc,
                                     const TStringView fieldName,
                                     rapidjson::Node& node);
#pragma endregion SerializeHeaders

#pragma region mark SerializeValues
    static void SerializePrimitiveObjectValue(const void* obj,
                                              Reflection::PrimitiveType type,
                                              rapidjson::Node& node);

    static void SerializePrimitiveArrayValue(const void* obj,
                                             Reflection::PrimitiveType type,
                                             rapidjson::Node& node);

    static void SerializeEnumObjectValue(const void* obj, rapidjson::Node& node, size_t size);

    static void SerializeEnumArrayValue(const void* obj, rapidjson::Node& node, size_t size);

    static void SerializeClassObjectFields(const void* obj,
                                           const TStringView fieldName,
                                           const Reflection::ClassDescription& classDesc,
                                           rapidjson::Node& node);

    static void SerializeClassArrayFields(const void* obj,
                                          const Reflection::ClassDescription& classDesc,
                                          rapidjson::Node& node);

    static void SerializeArrayObjectElements(const void* obj,
                                             const Reflection::ArrayDescription& arrayDesc,
                                             rapidjson::Node& outElements);
#pragma endregion SerializeValues

#pragma region mark SerializeObjects
    static void SerializePrimitiveObject(const void* obj,
                                         const TStringView fieldName,
                                         Reflection::PrimitiveType type,
                                         rapidjson::Node& node);

    static void SerializeEnumObject(const void* obj,
                                    const TStringView fieldName,
                                    const Reflection::EnumDescription& enumDesc,
                                    rapidjson::Node& outObject);

    static void SerializeClassObject(const void* obj,
                                     const TStringView fieldName,
                                     const Reflection::ClassDescription& classDesc,
                                     rapidjson::Node& node);

    static void SerializeArrayObject(const void* obj,
                                     const TStringView fieldName,
                                     const Reflection::ArrayDescription& arrayDesc,
                                     rapidjson::Node& outObjects);
#pragma endregion SerializeObjects

#pragma endregion SerializeForwardDecl


#pragma region mark DeserializeForwardDecl

    static void DeserializePrimitiveObject(const rapidjson::ConstNode& object, Reflection::PrimitiveType primitiveType, void* obj);

    static void DeserializeEnumObject(const rapidjson::ConstNode& object, const Reflection::EnumDescription& enumDesc, void* obj);

    static void DeserializeClassObject(const rapidjson::ConstNode& object, const Reflection::ClassDescription& classDesc, void* obj);

    static void DeserializeArrayObject(const rapidjson::ConstNode& object, const Reflection::ArrayDescription& arrayDesc, void* obj);
    
    static void DeserializePrimitiveArrayValue(const rapidjson::ConstNode& element, Reflection::PrimitiveType type, void* obj);
    
    static void DeserializeEnumArrayValue(const rapidjson::ConstNode& element, const Reflection::EnumDescription& enumDesc, void* obj);
    
    static void DeserializeClassArrayValue(const rapidjson::ConstNode& element, const Reflection::ClassDescription& classDesc, void* obj);
    
    static void DeserializeArrayElements(const rapidjson::ConstNode& array, const Reflection::ArrayDescription& arrayDesc, void* obj);

#pragma endregion DeserializeForwardDecl
};

JSONSerializer::JSONSerializer(FileStream& stream)
    : mHandle(new Impl(stream))
{
    
}

JSONSerializer::~JSONSerializer()
{
    if (mHandle)
    {
        delete mHandle;
    }
}

void JSONSerializer::Initialize(Engine* engine)
{
	// Custom serializers
	{
        mCustomObjectSerializers[Reflection::GetClass<Guid>().ResolveName()] = [](const void* obj,
            const TStringView fieldName,
            const Reflection::ClassDescription& classDesc,
			rapidjson::Node& node)
        {
            auto& guid = Reflection::Get<Guid>(obj);
            Impl::SerializeClassHeader(classDesc, fieldName, node);
			node.AddMember("Value", guid.ToString());
        };
        
        mCustomArraySerializers[Reflection::GetClass<Guid>().ResolveName()] = [](const void* obj,
            const Reflection::ClassDescription& classDesc,
			rapidjson::Node& node)
        {
            auto& guid = Reflection::Get<Guid>(obj);
            node.PushBack(rapidjson::Value(guid.ToString(), node.allocator));
        };
        
		mCustomObjectSerializers[Reflection::GetClass<TString>().ResolveName()] = [](const void* obj,
			const TStringView fieldName,
			const Reflection::ClassDescription& classDesc,
			rapidjson::Node& node)
		{
			const auto& str = Reflection::Get<TString>(obj);
            Impl::SerializeClassHeader(classDesc, fieldName, node);
			node.AddMember("Value", rapidjson::StringRef(str.c_str(), str.length()));
		};

		mCustomArraySerializers[Reflection::GetClass<TString>().ResolveName()] = [](const void* obj,
			const Reflection::ClassDescription& classDesc,
			rapidjson::Node& node)
		{
			const auto& str = Reflection::Get<TString>(obj);
			node.PushBack(rapidjson::Value(rapidjson::StringRef(str.c_str(), str.length())));
		};
        
        mCustomObjectSerializers[Reflection::GetClass<Filesystem::Path>().ResolveName()] = [](const void* obj,
            const TStringView fieldName,
            const Reflection::ClassDescription& classDesc,
			rapidjson::Node& node)
        {
            const auto& path = Reflection::Get<Filesystem::Path>(obj);
            const auto& pathStr = path.string();
            Impl::SerializeClassHeader(classDesc, fieldName, node);
			node.AddMember("Value", rapidjson::StringRef(pathStr.c_str(), pathStr.length()));
        };

        mCustomArraySerializers[Reflection::GetClass<Filesystem::Path>().ResolveName()] = [](const void* obj,
            const Reflection::ClassDescription& classDesc,
			rapidjson::Node& node)
        {
            const auto& path = Reflection::Get<Filesystem::Path>(obj);
            const auto& pathStr = path.string();
            node.PushBack(rapidjson::Value(rapidjson::StringRef(pathStr.c_str(), pathStr.length())));
        };

		mCustomObjectSerializers[Reflection::GetClass<TArray<uint8_t>>().ResolveName()] = [](const void* obj,
			const TStringView fieldName,
			const Reflection::ClassDescription& classDesc,
			rapidjson::Node& node)
		{
			const auto& arr = Reflection::Get<TArray<uint8_t>>(obj);
			const auto& arrDesc = Reflection::GetArray(classDesc.ContainerHash());
			auto containerDesc = Reflection::ArrayDescription(arrDesc.ResolveName(), arrDesc.ElementType(), arrDesc.ElementHash(), arr.size(), arrDesc.GetStride());
            Impl::SerializeArrayObject(arr.data(), fieldName, containerDesc, node);
		};

		mCustomArraySerializers[Reflection::GetClass<TArray<uint8_t>>().ResolveName()] = [](const void* obj,
			const Reflection::ClassDescription& classDesc,
			rapidjson::Node& node)
		{
			const auto& arr = Reflection::Get<TArray<uint8_t>>(obj);
			const auto& arrDesc = Reflection::GetArray(classDesc.ContainerHash());
			auto containerDesc = Reflection::ArrayDescription(arrDesc.ResolveName(), arrDesc.ElementType(), arrDesc.ElementHash(), arr.size(), arrDesc.GetStride());
            Impl::SerializeArrayObjectElements(arr.data(), containerDesc, node);
		};
	}
    
	// Custom deserializers
	{
        mCustomObjectDeserializers[Reflection::GetClass<Guid>().ResolveName()] = [](const rapidjson::ConstNode& node,
            const Reflection::ClassDescription& classDesc,
            void* obj)
        {
            if (node.object.HasMember("Value"))
            {
                Reflection::Get<Guid>(obj) = Guid(node.object["Value"].GetString());
            }
        };
        
        mCustomArrayDeserializers[Reflection::GetClass<Guid>().ResolveName()] = [](const rapidjson::ConstNode& node,
            const Reflection::ClassDescription& classDesc,
            void* obj)
        {
            Reflection::Get<Guid>(obj) = Guid(node.object.GetString());
        };
        
		mCustomObjectDeserializers[Reflection::GetClass<TString>().ResolveName()] = [](const rapidjson::ConstNode& node,
			const Reflection::ClassDescription& classDesc,
			void* obj)
		{
            if (node.object.HasMember("Value"))
            {
                Reflection::Get<TString>(obj) = TString(node.object["Value"].GetString());
            }
		};
        
        mCustomArrayDeserializers[Reflection::GetClass<TString>().ResolveName()] = [](const rapidjson::ConstNode& node,
            const Reflection::ClassDescription& classDesc,
            void* obj)
        {
            Reflection::Get<TString>(obj) = TString(node.object.GetString());
        };
        
        mCustomObjectDeserializers[Reflection::GetClass<Filesystem::Path>().ResolveName()] = [](const rapidjson::ConstNode& node,
            const Reflection::ClassDescription& classDesc,
            void* obj)
        {
            if (node.object.HasMember("Value"))
            {
                Reflection::Get<Filesystem::Path>(obj) = TString(node.object["Value"].GetString());
            }
        };
        
        mCustomArrayDeserializers[Reflection::GetClass<Filesystem::Path>().ResolveName()] = [](const rapidjson::ConstNode& node,
            const Reflection::ClassDescription& classDesc,
            void* obj)
        {
            Reflection::Get<Filesystem::Path>(obj) = TString(node.object.GetString());
        };

		mCustomObjectDeserializers[Reflection::GetClass<TArray<uint8_t>>().ResolveName()] = [](const rapidjson::ConstNode& node,
			const Reflection::ClassDescription& classDesc,
			void* obj)
		{
			if (node.object.HasMember("Elements"))
			{
				const auto& elements = node.object["Elements"].GetArray();

				const auto& arrDesc = Reflection::GetArray(classDesc.ContainerHash());
				auto containerDesc = Reflection::ArrayDescription(arrDesc.ResolveName(), arrDesc.ElementType(), arrDesc.ElementHash(), elements.Size(), arrDesc.GetStride());

				auto& arr = Reflection::Get<TArray<uint8_t>>(obj);
				arr.resize(elements.Size() * arrDesc.GetStride());

				rapidjson::ConstNode elementsNode(elements);
				Impl::DeserializeArrayElements(elementsNode, containerDesc, arr.data());
			}
		};
        
        mCustomArrayDeserializers[Reflection::GetClass<TArray<uint8_t>>().ResolveName()] = [](const rapidjson::ConstNode& node,
            const Reflection::ClassDescription& classDesc,
            void* obj)
        {
            const auto& arrDesc = Reflection::GetArray(classDesc.ContainerHash());
            auto containerDesc = Reflection::ArrayDescription(arrDesc.ResolveName(), arrDesc.ElementType(), arrDesc.ElementHash(), node.object.Size(), arrDesc.GetStride());

            auto& arr = Reflection::Get<TArray<uint8_t>>(obj);
            arr.resize(node.object.Size() * arrDesc.GetStride());
            Impl::DeserializeArrayElements(node, containerDesc, arr.data());
        };
	}
}

void JSONSerializer::Shutdown()
{
    mCustomObjectSerializers.clear();
    mCustomArraySerializers.clear();
	mCustomObjectDeserializers.clear();
	mCustomArrayDeserializers.clear();
}

JSONHeader JSONSerializer::ParseHeader()
{
	rapidjson::IStreamWrapper ss(mHandle->stream);
	rapidjson::Document document(rapidjson::kObjectType);
	document.ParseStream(ss);

	JSONHeader header;
	header.guid = TString(document["TypeGuid"].GetString());
	header.name = TString(document["TypeName"].GetString());
	
	TString kind = document["Kind"].GetString();
	if (kind == "Primitive")
	{
		header.kind = Reflection::FieldType::Primitive;
	}
	else if (kind == "Array")
	{
		header.kind = Reflection::FieldType::Array;
	}
	else if (kind == "Class")
	{
		header.kind = Reflection::FieldType::Class;
	}
	else if (kind == "Enum")
	{
		header.kind = Reflection::FieldType::Enum;
	}
	else
	{
		header.kind = Reflection::FieldType::Invalid;
	}

	constexpr auto version = Reflection::Attribute::Version::description;
	if (document.HasMember(version.tag))
	{
		header.version = document[version.tag].Get<uint32_t>();
	}

	return header;
}

void JSONSerializer::Serialize(const void* obj, const Reflection::ClassDescription& classDesc)
{
	rapidjson::Document document(rapidjson::kObjectType);
	rapidjson::Node root(document, document.GetAllocator());
	Serialize(obj, classDesc, root);
	
    rapidjson::OStreamWrapper ss(mHandle->stream);
    rapidjson::PrettyWriter writer(ss);
    writer.SetFormatOptions(rapidjson::PrettyFormatOptions::kFormatSingleLineArray);
    writer.SetMaxDecimalPlaces(6);
    writer.SetIndent('\t', 1);
    document.Accept(writer);
}

void JSONSerializer::Serialize(const void* obj, const Reflection::ClassDescription& classDesc, rapidjson::Node& root)
{
	if (TryCustomObjectSerializer(obj, "", classDesc, root) == false)
	{
		mHandle->SerializeClassObject(obj, "", classDesc, root);
	}
}

void JSONSerializer::Deserialize(const Reflection::ClassDescription& classDesc, void* obj)
{
	rapidjson::Document document(rapidjson::kObjectType);
    rapidjson::IStreamWrapper ss(mHandle->stream);
	document.ParseStream(ss);

	rapidjson::ConstNode root(document);
	Deserialize(classDesc, obj, root);
}

void JSONSerializer::Deserialize(const Reflection::ClassDescription& classDesc, void* obj, const rapidjson::ConstNode& root)
{
	if (TryCustomObjectDeserializer(root, classDesc, obj) == false)
	{
		mHandle->DeserializeClassObject(root, classDesc, obj);
	}
}

bool JSONSerializer::TryCustomObjectSerializer(const void* obj,
											   const TStringView fieldName,
											   const Reflection::ClassDescription& classDesc,
											   rapidjson::Node& node)
{
	auto it = mCustomObjectSerializers.find(classDesc.ResolveName());
	if (it != mCustomObjectSerializers.end())
	{
		it->second(obj, fieldName, classDesc, node);
		return true;
	}
	return false;
}

bool JSONSerializer::TryCustomArraySerializer(const void* obj,
											  const Reflection::ClassDescription& classDesc,
											  rapidjson::Node& node)
{
	auto it = mCustomArraySerializers.find(classDesc.ResolveName());
	if (it != mCustomArraySerializers.end())
	{
		it->second(obj, classDesc, node);
		return true;
	}
	return false;
}

bool JSONSerializer::TryCustomObjectDeserializer(const rapidjson::ConstNode& node,
												 const Reflection::ClassDescription& classDesc,
												 void* obj)
{
	auto it = mCustomObjectDeserializers.find(classDesc.ResolveName());
	if (it != mCustomObjectDeserializers.end())
	{
		it->second(node, classDesc, obj);
		return true;
	}
	return false;
}

bool JSONSerializer::TryCustomArrayDeserializer(const rapidjson::ConstNode& node,
                                                const Reflection::ClassDescription& classDesc,
                                                void* obj)
{
    auto it = mCustomArrayDeserializers.find(classDesc.ResolveName());
    if (it != mCustomArrayDeserializers.end())
    {
        it->second(node, classDesc, obj);
        return true;
    }
    return false;
}

#pragma region mark SerializeFunctionsImpl

#pragma region mark SerializeHeaders
void JSONSerializer::Impl::SerializePrimitiveHeader(Reflection::PrimitiveType type,
                                                    const TStringView fieldName,
                                                    rapidjson::Node& node)
{
    node.AddMember("Kind", rapidjson::StringRef("Primitive"));
    node.AddMember("TypeName", rapidjson::StringRef(Reflection::Database::GetPrimitiveName(type).data()));
    
    if (not fieldName.empty())
    {
        node.AddMember("FieldName", rapidjson::StringRef(fieldName.data()));
    }
}

void JSONSerializer::Impl::SerializeEnumHeader(const Reflection::EnumDescription& enumDesc,
                                               const TStringView fieldName,
                                               rapidjson::Node& node)
{
    node.AddMember("Kind", rapidjson::StringRef("Enum"));
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

void JSONSerializer::Impl::SerializeClassHeader(const Reflection::ClassDescription& classDesc,
                                                const TStringView fieldName,
                                                rapidjson::Node& node)
{
    node.AddMember("Kind", rapidjson::StringRef("Class"));
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

void JSONSerializer::Impl::SerializeArrayHeader(const Reflection::ArrayDescription& arrayDesc,
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
    if (not fieldName.empty())
    {
        node.AddMember("FieldName", rapidjson::StringRef(fieldName.data()));
    }
}
#pragma endregion SerializeHeaders

#pragma region mark SerializeValues
void JSONSerializer::Impl::SerializePrimitiveObjectValue(const void* obj,
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

void JSONSerializer::Impl::SerializePrimitiveArrayValue(const void* obj,
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

void JSONSerializer::Impl::SerializeEnumObjectValue(const void* obj, rapidjson::Node& node, size_t size)
{
    GLEAM_ASSERT(size <= sizeof(int64_t), "JSONSerializer: Enum is larger than 8 bytes");
    int64_t value = 0;
    memcpy(&value, obj, size);
    node.AddMember("Value", value);
}

void JSONSerializer::Impl::SerializeEnumArrayValue(const void* obj, rapidjson::Node& node, size_t size)
{
    int64_t value = 0;
    memcpy(&value, obj, size);
    node.PushBack(rapidjson::Value(value));
}

void JSONSerializer::Impl::SerializeClassObjectFields(const void* obj,
                                                      const TStringView fieldName,
                                                      const Reflection::ClassDescription& classDesc,
                                                      rapidjson::Node& outFields)
{
    for (const auto& baseClass : classDesc.ResolveBaseClasses())
    {
		auto baseObject = rapidjson::Value(rapidjson::kObjectType);
		auto baseNode = rapidjson::Node(baseObject, outFields.allocator);
        SerializeClassObject(obj, fieldName, baseClass, baseNode);
		outFields.PushBack(baseNode.object);
    }
    
    for (const auto& field : classDesc.ResolveFields())
    {
        if (field.HasAttribute<Reflection::Attribute::Serializable>())
        {
            auto fieldObject = rapidjson::Value(rapidjson::kObjectType);
            auto fieldNode = rapidjson::Node(fieldObject, outFields.allocator);
            switch (field.GetType())
            {
                case Reflection::FieldType::Class:
                {
					const auto& classField = field.GetField<Reflection::ClassField>();
					const auto& fieldDesc = Reflection::GetClass(classField.hash);
                    if (JSONSerializer::TryCustomObjectSerializer(OffsetPointer(obj, classField.offset), field.ResolveName(), fieldDesc, fieldNode) == false)
                    {
                        SerializeClassObject(OffsetPointer(obj, classField.offset), field.ResolveName(), fieldDesc, fieldNode);
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
                    SerializeEnumObject(OffsetPointer(obj, enumField.offset), field.ResolveName(), enumDesc, fieldNode);
                    break;
                }
                case Reflection::FieldType::Primitive:
                {
					const auto& primitiveField = field.GetField<Reflection::PrimitiveField>();
                    auto primitiveType = primitiveField.primitive;
                    SerializePrimitiveObject(OffsetPointer(obj, primitiveField.offset), field.ResolveName(), primitiveType, fieldNode);
                    break;
                }
                default:
                    continue;
            }
            outFields.PushBack(fieldNode.object);
        }
    }
}

void JSONSerializer::Impl::SerializeClassArrayFields(const void* obj,
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
					const auto& classField = field.GetField<Reflection::ClassField>();
					const auto& fieldDesc = Reflection::GetClass(classField.hash);
                    if (JSONSerializer::TryCustomArraySerializer(OffsetPointer(obj, classField.offset), fieldDesc, outFields) == false)
                    {
                        auto elements = rapidjson::Value(rapidjson::kArrayType);
                        auto elementsNode = rapidjson::Node(elements, outFields.allocator);
                        SerializeClassArrayFields(OffsetPointer(obj, classField.offset), fieldDesc, elementsNode);
                        outFields.PushBack(elementsNode.object);
                    }
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
                    SerializeEnumArrayValue(OffsetPointer(obj, enumField.offset), outFields, enumField.size);
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
				{
					GLEAM_ASSERT(false, "JSONSerializer: Unknown object kind");
					continue;
				}
            }
        }
    }
}

void JSONSerializer::Impl::SerializeArrayObjectElements(const void* obj,
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
            SerializeEnumArrayValue(OffsetPointer(obj, elementOffset), outElements, enumDesc.GetSize());
        }
    }
	else
	{
		GLEAM_ASSERT(false, "JSONSerializer: Unknown object kind");
		return;
	}
}
#pragma endregion SerializeValues

#pragma region mark SerializeObjects
void JSONSerializer::Impl::SerializePrimitiveObject(const void* obj,
                                                    const TStringView fieldName,
                                                    Reflection::PrimitiveType type,
                                                    rapidjson::Node& outObject)
{
    SerializePrimitiveHeader(type, fieldName, outObject);
    SerializePrimitiveObjectValue(obj, type, outObject);
}

void JSONSerializer::Impl::SerializeEnumObject(const void* obj,
                                               const TStringView fieldName,
                                               const Reflection::EnumDescription& enumDesc,
                                               rapidjson::Node& outObject)
{
    SerializeEnumHeader(enumDesc, fieldName, outObject);
    SerializeEnumObjectValue(obj, outObject, enumDesc.GetSize());
}

void JSONSerializer::Impl::SerializeClassObject(const void* obj,
                                                const TStringView fieldName,
                                                const Reflection::ClassDescription& classDesc,
                                                rapidjson::Node& outObjects)
{
    rapidjson::Value fields(rapidjson::kArrayType);
    rapidjson::Node fieldsNode(fields, outObjects.allocator);
    SerializeClassObjectFields(obj, fieldName, classDesc, fieldsNode);
    if (fields.Size() > 0)
	{
        SerializeClassHeader(classDesc, fieldName, outObjects);
		outObjects.AddMember("Fields", fields);
    }
}

void JSONSerializer::Impl::SerializeArrayObject(const void* obj,
                                                const TStringView fieldName,
                                                const Reflection::ArrayDescription& arrayDesc,
                                                rapidjson::Node& outObjects)
{
    rapidjson::Value elements(rapidjson::kArrayType);
    rapidjson::Node elementsNode(elements, outObjects.allocator);
    SerializeArrayObjectElements(obj, arrayDesc, elementsNode);
    if (elements.Size() > 0)
    {
        SerializeArrayHeader(arrayDesc, fieldName, outObjects);
		outObjects.AddMember("Elements", elements);
    }
}
#pragma endregion SerializeObjects

#pragma endregion SerializeFunctionsImpl

#pragma region mark DeserializeFunctionsImpl

template<typename T, std::enable_if_t<Reflection::Traits::IsPrimitive<T>::value, bool> = true>
constexpr void DeserializeValue(const rapidjson::Value& object, void* obj)
{
    if (object.HasMember("Value"))
    {
        Reflection::Get<T>(obj) = object["Value"].Get<T>();
    }
}

void JSONSerializer::Impl::DeserializePrimitiveObject(const rapidjson::ConstNode& node,
                                                      Reflection::PrimitiveType type,
                                                      void* obj)
{
	switch (type)
	{
		case Reflection::PrimitiveType::Bool:
			DeserializeValue<bool>(node.object, obj);
			break;
		case Reflection::PrimitiveType::Int8:
			DeserializeValue<int8_t>(node.object, obj);
			break;
		case Reflection::PrimitiveType::WChar:
			DeserializeValue<wchar_t>(node.object, obj);
			break;
		case Reflection::PrimitiveType::Char:
			DeserializeValue<char>(node.object, obj);
			break;
		case Reflection::PrimitiveType::Int16:
			DeserializeValue<int16_t>(node.object, obj);
			break;
		case Reflection::PrimitiveType::Int32:
			DeserializeValue<int32_t>(node.object, obj);
			break;
		case Reflection::PrimitiveType::Int64:
			DeserializeValue<int64_t>(node.object, obj);
			break;
		case Reflection::PrimitiveType::UInt8:
			DeserializeValue<uint8_t>(node.object, obj);
			break;
		case Reflection::PrimitiveType::UInt16:
			DeserializeValue<uint16_t>(node.object, obj);
			break;
		case Reflection::PrimitiveType::UInt32:
			DeserializeValue<uint32_t>(node.object, obj);
			break;
		case Reflection::PrimitiveType::UInt64:
			DeserializeValue<uint64_t>(node.object, obj);
			break;
		case Reflection::PrimitiveType::Float:
			DeserializeValue<float>(node.object, obj);
			break;
		case Reflection::PrimitiveType::Double:
			DeserializeValue<double>(node.object, obj);
			break;
		default:
			GLEAM_ASSERT(false, "JSONSerializer: Unknown primitive type");
			break;
	}
}

void JSONSerializer::Impl::DeserializeEnumObject(const rapidjson::ConstNode& node,
                                                 const Reflection::EnumDescription& enumDesc,
                                                 void* obj)
{
	int64_t value = 0;
	DeserializeValue<int64_t>(node.object, &value);
	memcpy(obj, &value, enumDesc.GetSize());
}

void JSONSerializer::Impl::DeserializeClassObject(const rapidjson::ConstNode& node,
                                                  const Reflection::ClassDescription& classDesc,
                                                  void* obj)
{
    if (node.object.HasMember("Fields") == false)
    {
        return;
    }
    
	uint32_t fieldIdx = 0;
    auto fields = node.object["Fields"].GetArray();
	for (const auto& base : classDesc.ResolveBaseClasses())
	{
		rapidjson::ConstNode fieldNode(fields[fieldIdx++]);
		DeserializeClassObject(fieldNode, base, obj);
	}

	for (const auto& fieldDesc : classDesc.ResolveFields())
	{
		rapidjson::ConstNode fieldNode(fields[fieldIdx++]);
		if (fieldDesc.GetType() == Reflection::FieldType::Primitive)
		{
			const auto& primitiveField = fieldDesc.GetField<Reflection::PrimitiveField>();
			DeserializePrimitiveObject(fieldNode, primitiveField.primitive, OffsetPointer(obj, primitiveField.offset));
		}
		else if (fieldDesc.GetType() == Reflection::FieldType::Array)
		{
			const auto& arrayField = fieldDesc.GetField<Reflection::ArrayField>();
			const auto& desc = Reflection::GetArray(arrayField.hash);
			DeserializeArrayObject(fieldNode, desc, OffsetPointer(obj, arrayField.offset));
		}
		else if (fieldDesc.GetType() == Reflection::FieldType::Class)
		{
			const auto& classField = fieldDesc.GetField<Reflection::ClassField>();
			const auto& desc = Reflection::GetClass(classField.hash);
			if (JSONSerializer::TryCustomObjectDeserializer(fieldNode, desc, OffsetPointer(obj, classField.offset)) == false)
			{
				DeserializeClassObject(fieldNode, desc, OffsetPointer(obj, classField.offset));
			}
		}
		else if (fieldDesc.GetType() == Reflection::FieldType::Enum)
		{
			const auto& enumField = fieldDesc.GetField<Reflection::EnumField>();
			const auto& desc = Reflection::GetEnum(enumField.hash);
			DeserializeEnumObject(fieldNode, desc, OffsetPointer(obj, enumField.offset));
		}
		else
		{
			GLEAM_ASSERT(false, "JSONSerializer: Unknown object kind");
			continue;
		}
	}
}

void JSONSerializer::Impl::DeserializeArrayObject(const rapidjson::ConstNode& node,
                                                  const Reflection::ArrayDescription& arrayDesc,
                                                  void* obj)
{
    if (node.object.HasMember("Elements") == false)
    {
        return;
    }
    
	switch (arrayDesc.ElementType())
	{
		case Reflection::FieldType::Primitive:
		{
			auto primitiveType = Reflection::Database::GetPrimitiveType(arrayDesc.ElementHash());

			size_t offset = 0;
			for (auto& element : node.object["Elements"].GetArray())
			{
				rapidjson::ConstNode elementNode(element);
				DeserializePrimitiveObject(elementNode, primitiveType, OffsetPointer(obj, offset));
				offset += arrayDesc.GetStride();
			}
			return;
		}
		case Reflection::FieldType::Array:
		{
			const auto& innerDesc = Reflection::GetArray(arrayDesc.ElementHash());

			size_t offset = 0;
			for (auto& element : node.object["Elements"].GetArray())
			{
				rapidjson::ConstNode elementNode(element);
				DeserializeArrayObject(elementNode, innerDesc, OffsetPointer(obj, offset));
				offset += arrayDesc.GetStride();
			}
			return;
		}
		case Reflection::FieldType::Class:
		{
			const auto& classDesc = Reflection::GetClass(arrayDesc.ElementHash());

			size_t offset = 0;
			for (auto& element : node.object["Elements"].GetArray())
			{
				rapidjson::ConstNode elementNode(element);
				if (JSONSerializer::TryCustomObjectDeserializer(elementNode, classDesc, OffsetPointer(obj, offset)) == false)
				{
					DeserializeClassObject(elementNode, classDesc, OffsetPointer(obj, offset));
				}
				offset += arrayDesc.GetStride();
			}
			return;
		}
		case Reflection::FieldType::Enum:
		{
			const auto& enumDesc = Reflection::GetEnum(arrayDesc.ElementHash());

			size_t offset = 0;
			for (auto& element : node.object["Elements"].GetArray())
			{
				rapidjson::ConstNode elementNode(element);
				DeserializeEnumObject(elementNode, enumDesc, OffsetPointer(obj, offset));
				offset += arrayDesc.GetStride();
			}
			return;
		}
		default:
		{
			GLEAM_ASSERT(false, "JSONSerializer: Unknown object kind");
			return;
		}
	}
}

void JSONSerializer::Impl::DeserializePrimitiveArrayValue(const rapidjson::ConstNode& node,
                                                          Reflection::PrimitiveType type,
                                                          void* obj)
{
    switch (type)
    {
        case Reflection::PrimitiveType::Bool:
            Reflection::Get<bool>(obj) = node.object.Get<bool>();
            break;
        case Reflection::PrimitiveType::Int8:
            Reflection::Get<int8_t>(obj) = node.object.Get<int8_t>();
            break;
        case Reflection::PrimitiveType::WChar:
            Reflection::Get<wchar_t>(obj) = node.object.Get<wchar_t>();
            break;
        case Reflection::PrimitiveType::Char:
            Reflection::Get<char>(obj) = node.object.Get<char>();
            break;
        case Reflection::PrimitiveType::Int16:
            Reflection::Get<int16_t>(obj) = node.object.Get<int16_t>();
            break;
        case Reflection::PrimitiveType::Int32:
            Reflection::Get<int32_t>(obj) = node.object.Get<int32_t>();
            break;
        case Reflection::PrimitiveType::Int64:
            Reflection::Get<int64_t>(obj) = node.object.Get<int64_t>();
            break;
        case Reflection::PrimitiveType::UInt8:
            Reflection::Get<uint8_t>(obj) = node.object.Get<uint8_t>();
            break;
        case Reflection::PrimitiveType::UInt16:
            Reflection::Get<uint16_t>(obj) = node.object.Get<uint16_t>();
            break;
        case Reflection::PrimitiveType::UInt32:
            Reflection::Get<uint32_t>(obj) = node.object.Get<uint32_t>();
            break;
        case Reflection::PrimitiveType::UInt64:
            Reflection::Get<uint64_t>(obj) = node.object.Get<uint64_t>();
            break;
        case Reflection::PrimitiveType::Float:
            Reflection::Get<float>(obj) = node.object.Get<float>();
            break;
        case Reflection::PrimitiveType::Double:
            Reflection::Get<double>(obj) = node.object.Get<double>();
            break;
        default:
            GLEAM_ASSERT(false, "JSONSerializer: Unknown primitive type");
            break;
    }
}

void JSONSerializer::Impl::DeserializeEnumArrayValue(const rapidjson::ConstNode& node,
                                                     const Reflection::EnumDescription& enumDesc,
                                                     void* obj)
{
    Reflection::Get<int64_t>(obj) = node.object.Get<int64_t>();
}

void JSONSerializer::Impl::DeserializeClassArrayValue(const rapidjson::ConstNode& node,
                                                      const Reflection::ClassDescription& classDesc,
                                                      void* obj)
{
    uint32_t fieldIdx = 0;
    auto fields = node.object.GetArray();
    for (const auto& base : classDesc.ResolveBaseClasses())
    {
		rapidjson::ConstNode fieldNode(fields[fieldIdx++]);
        DeserializeClassArrayValue(fieldNode, base, obj);
    }

    for (const auto& fieldDesc : classDesc.ResolveFields())
    {
		rapidjson::ConstNode fieldNode(fields[fieldIdx++]);
        if (fieldDesc.GetType() == Reflection::FieldType::Primitive)
        {
            const auto& primitiveField = fieldDesc.GetField<Reflection::PrimitiveField>();
            DeserializePrimitiveArrayValue(fieldNode, primitiveField.primitive, OffsetPointer(obj, primitiveField.offset));
        }
        else if (fieldDesc.GetType() == Reflection::FieldType::Array)
        {
            const auto& arrayField = fieldDesc.GetField<Reflection::ArrayField>();
            const auto& desc = Reflection::GetArray(arrayField.hash);
            DeserializeArrayElements(fieldNode, desc, OffsetPointer(obj, arrayField.offset));
        }
        else if (fieldDesc.GetType() == Reflection::FieldType::Class)
        {
            const auto& classField = fieldDesc.GetField<Reflection::ClassField>();
            const auto& desc = Reflection::GetClass(classField.hash);
            if (JSONSerializer::TryCustomArrayDeserializer(fieldNode, desc, OffsetPointer(obj, classField.offset)) == false)
            {
                DeserializeClassArrayValue(fieldNode, desc, OffsetPointer(obj, classField.offset));
            }
        }
        else if (fieldDesc.GetType() == Reflection::FieldType::Enum)
        {
            const auto& enumField = fieldDesc.GetField<Reflection::EnumField>();
            const auto& desc = Reflection::GetEnum(enumField.hash);
            DeserializeEnumArrayValue(fieldNode, desc, OffsetPointer(obj, enumField.offset));
        }
        else
        {
            GLEAM_ASSERT(false, "JSONSerializer: Unknown object kind");
            continue;
        }
    }
}

void JSONSerializer::Impl::DeserializeArrayElements(const rapidjson::ConstNode& node,
                                                    const Reflection::ArrayDescription& arrayDesc,
                                                    void* obj)
{
    switch (arrayDesc.ElementType())
    {
        case Reflection::FieldType::Primitive:
        {
            auto primitiveType = Reflection::Database::GetPrimitiveType(arrayDesc.ElementHash());

            size_t offset = 0;
            for (auto& element : node.object.GetArray())
            {
				rapidjson::ConstNode elementNode(element);
                DeserializePrimitiveArrayValue(elementNode, primitiveType, OffsetPointer(obj, offset));
                offset += arrayDesc.GetStride();
            }
            return;
        }
        case Reflection::FieldType::Array:
        {
            const auto& innerDesc = Reflection::GetArray(arrayDesc.ElementHash());

            size_t offset = 0;
            for (auto& element : node.object.GetArray())
            {
				rapidjson::ConstNode elementNode(element);
                DeserializeArrayElements(elementNode, innerDesc, OffsetPointer(obj, offset));
                offset += arrayDesc.GetStride();
            }
            return;
        }
        case Reflection::FieldType::Class:
        {
            const auto& classDesc = Reflection::GetClass(arrayDesc.ElementHash());

            size_t offset = 0;
            for (auto& element : node.object.GetArray())
            {
				rapidjson::ConstNode elementNode(element);
                if (JSONSerializer::TryCustomArrayDeserializer(elementNode, classDesc, OffsetPointer(obj, offset)) == false)
                {
                    DeserializeClassArrayValue(elementNode, classDesc, OffsetPointer(obj, offset));
                }
                offset += arrayDesc.GetStride();
            }
            return;
        }
        case Reflection::FieldType::Enum:
        {
            const auto& enumDesc = Reflection::GetEnum(arrayDesc.ElementHash());

            size_t offset = 0;
            for (auto& element : node.object.GetArray())
            {
				rapidjson::ConstNode elementNode(element);
                DeserializeEnumArrayValue(elementNode, enumDesc, OffsetPointer(obj, offset));
                offset += arrayDesc.GetStride();
            }
            return;
        }
        default:
        {
            GLEAM_ASSERT(false, "JSONSerializer: Unknown object kind");
            return;
        }
    }
}

#pragma endregion DeserializeFunctionsImpl
