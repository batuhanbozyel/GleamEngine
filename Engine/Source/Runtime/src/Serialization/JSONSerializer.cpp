#include "gpch.h"
#include "JSONSerializer.h"

#define RAPIDJSON_HAS_STDSTRING 1
#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/ostreamwrapper.h>

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

	Value& operator[](const char* name) const
	{
		return object[name];
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

struct JSONSerializer::Impl
{
    rapidjson::Document document;
	FileStream& stream;
    
    Impl(FileStream& stream)
        : stream(stream)
        , document(rapidjson::kObjectType)
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

    static void SerializeEnumObjectValue(const void* obj, rapidjson::Node& node);

    static void SerializeEnumArrayValue(const void* obj, rapidjson::Node& node);

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

    static void DeserializePrimitiveObject(const rapidjson::Value& object, Reflection::PrimitiveType primitiveType, void* obj);

    static void DeserializeEnumObject(const rapidjson::Value& object, const Reflection::EnumDescription& enumDesc, void* obj);

    static void DeserializeClassObject(const rapidjson::Value& object, const Reflection::ClassDescription& classDesc, void* obj);

    static void DeserializeArrayObject(const rapidjson::Value& object, const Reflection::ArrayDescription& arrayDesc, void* obj);

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

void JSONSerializer::Initialize()
{
	// Custom serializers
	{
        mCustomObjectSerializers[Reflection::GetClass<Guid>().ResolveName()] = [](const void* obj,
            const TStringView fieldName,
            const Reflection::ClassDescription& classDesc,
            void* userData)
        {
            auto guidStr = Reflection::Get<Guid>(obj).ToString();
            auto& outObject = Reflection::Get<rapidjson::Node>(userData);
            Impl::SerializeClassHeader(classDesc, fieldName, outObject);

            rapidjson::Value object(rapidjson::kObjectType);
            rapidjson::Node node(object, outObject.allocator);
            outObject.AddMember("Value", rapidjson::StringRef(guidStr.c_str(), guidStr.length()));
        };
        
        mCustomArraySerializers[Reflection::GetClass<Guid>().ResolveName()] = [](const void* obj,
            const Reflection::ClassDescription& classDesc,
            void* userData)
        {
            auto guidStr = Reflection::Get<Guid>(obj).ToString();
            auto& outObject = Reflection::Get<rapidjson::Node>(userData);
            outObject.PushBack(rapidjson::Value(rapidjson::StringRef(guidStr.c_str(), guidStr.length())));
        };
        
		mCustomObjectSerializers[Reflection::GetClass<TString>().ResolveName()] = [](const void* obj,
			const TStringView fieldName,
			const Reflection::ClassDescription& classDesc,
			void* userData)
		{
			const auto& str = Reflection::Get<TString>(obj);
			auto& outObject = Reflection::Get<rapidjson::Node>(userData);
            Impl::SerializeClassHeader(classDesc, fieldName, outObject);

			rapidjson::Value object(rapidjson::kObjectType);
			rapidjson::Node node(object, outObject.allocator);
			outObject.AddMember("Value", rapidjson::StringRef(str.c_str(), str.length()));
		};

		mCustomArraySerializers[Reflection::GetClass<TString>().ResolveName()] = [](const void* obj,
			const Reflection::ClassDescription& classDesc,
			void* userData)
		{
			const auto& str = Reflection::Get<TString>(obj);
			auto& outObject = Reflection::Get<rapidjson::Node>(userData);
			outObject.PushBack(rapidjson::Value(rapidjson::StringRef(str.c_str(), str.length())));
		};
        
        mCustomObjectSerializers[Reflection::GetClass<Filesystem::path>().ResolveName()] = [](const void* obj,
            const TStringView fieldName,
            const Reflection::ClassDescription& classDesc,
            void* userData)
        {
            const auto& path = Reflection::Get<Filesystem::path>(obj);
            const auto& pathStr = path.string();
            auto& outObject = Reflection::Get<rapidjson::Node>(userData);
            Impl::SerializeClassHeader(classDesc, fieldName, outObject);

            rapidjson::Value object(rapidjson::kObjectType);
            rapidjson::Node node(object, outObject.allocator);
            outObject.AddMember("Value", rapidjson::StringRef(pathStr.c_str(), pathStr.length()));
        };

        mCustomArraySerializers[Reflection::GetClass<Filesystem::path>().ResolveName()] = [](const void* obj,
            const Reflection::ClassDescription& classDesc,
            void* userData)
        {
            const auto& path = Reflection::Get<Filesystem::path>(obj);
            const auto& pathStr = path.string();
            
            auto& outObject = Reflection::Get<rapidjson::Node>(userData);
            outObject.PushBack(rapidjson::Value(rapidjson::StringRef(pathStr.c_str(), pathStr.length())));
        };

		mCustomObjectSerializers[Reflection::GetClass<TArray<uint8_t>>().ResolveName()] = [](const void* obj,
			const TStringView fieldName,
			const Reflection::ClassDescription& classDesc,
			void* userData)
		{
			const auto& arr = Reflection::Get<TArray<uint8_t>>(obj);
			auto& outObject = Reflection::Get<rapidjson::Node>(userData);
			const auto& arrDesc = Reflection::GetArray(classDesc.ContainerHash());
			auto containerDesc = Reflection::ArrayDescription(arrDesc.ResolveName(), arrDesc.ElementType(), arrDesc.ElementHash(), arr.size(), arrDesc.GetStride());
            Impl::SerializeArrayObject(arr.data(), fieldName, containerDesc, outObject);
		};

		mCustomArraySerializers[Reflection::GetClass<TArray<uint8_t>>().ResolveName()] = [](const void* obj,
			const Reflection::ClassDescription& classDesc,
			void* userData)
		{
			const auto& arr = Reflection::Get<TArray<uint8_t>>(obj);
			auto& outObject = Reflection::Get<rapidjson::Node>(userData);
			const auto& arrDesc = Reflection::GetArray(classDesc.ContainerHash());
			auto containerDesc = Reflection::ArrayDescription(arrDesc.ResolveName(), arrDesc.ElementType(), arrDesc.ElementHash(), arr.size(), arrDesc.GetStride());
            Impl::SerializeArrayObjectElements(arr.data(), containerDesc, outObject);
		};
	}

	// Custom deserializers
	{
        mCustomObjectDeserializers[Reflection::GetClass<Guid>().ResolveName()] = [](const void* userData,
            const Reflection::ClassDescription& classDesc,
            void* obj)
        {
            const auto& value = Reflection::Get<rapidjson::Value>(userData);
            if (value.HasMember("Value"))
            {
                Reflection::Get<Guid>(obj) = Guid(value["Value"].GetString());
            }
        };
        
		mCustomObjectDeserializers[Reflection::GetClass<TString>().ResolveName()] = [](const void* userData,
			const Reflection::ClassDescription& classDesc,
			void* obj)
		{
			const auto& value = Reflection::Get<rapidjson::Value>(userData);
            if (value.HasMember("Value"))
            {
                Reflection::Get<TString>(obj) = value["Value"].GetString();
            }
		};
        
        mCustomObjectDeserializers[Reflection::GetClass<Filesystem::path>().ResolveName()] = [](const void* userData,
            const Reflection::ClassDescription& classDesc,
            void* obj)
        {
            const auto& value = Reflection::Get<rapidjson::Value>(userData);
            if (value.HasMember("Value"))
            {
                Reflection::Get<Filesystem::path>(obj) = value["Value"].GetString();
            }
        };

		mCustomObjectDeserializers[Reflection::GetClass<TArray<uint8_t>>().ResolveName()] = [](const void* userData,
			const Reflection::ClassDescription& classDesc,
			void* obj)
		{
			const auto& value = Reflection::Get<rapidjson::Value>(userData);
            if (value.HasMember("Elements"))
            {
                const auto& elements = value["Elements"].GetArray();

                const auto& arrDesc = Reflection::GetArray(classDesc.ContainerHash());
                auto containerDesc = Reflection::ArrayDescription(arrDesc.ResolveName(), arrDesc.ElementType(), arrDesc.ElementHash(), elements.Size(), arrDesc.GetStride());

                auto& arr = Reflection::Get<TArray<uint8_t>>(obj);
                arr.resize(elements.Size() * arrDesc.GetStride());

                size_t offset = 0;
                for (const auto& element : elements)
                {
                    Impl::DeserializeArrayObject(value, containerDesc, OffsetPointer(arr.data(), offset));
                    offset += arrDesc.GetStride();
                }
            }
		};
	}
}

void JSONSerializer::Shutdown()
{
    mCustomObjectSerializers.clear();
    mCustomArraySerializers.clear();
}

void JSONSerializer::Serialize(const void* obj, const Reflection::ClassDescription& classDesc)
{
	rapidjson::Node root(mHandle->document, mHandle->document.GetAllocator());
	if (TryCustomObjectSerializer(obj, "", classDesc, &root) == false)
	{
        mHandle->SerializeClassObject(obj, "", classDesc, root);
	}
	
    rapidjson::OStreamWrapper ss(mHandle->stream);
    rapidjson::PrettyWriter writer(ss);
    writer.SetFormatOptions(rapidjson::PrettyFormatOptions::kFormatSingleLineArray);
    writer.SetMaxDecimalPlaces(6);
    writer.SetIndent('\t', 1);
    mHandle->document.Accept(writer);
}

void JSONSerializer::Deserialize(const Reflection::ClassDescription& desc, void* obj)
{
    rapidjson::IStreamWrapper ss(mHandle->stream);
	mHandle->document.ParseStream(ss);

	if (TryCustomObjectDeserializer(&mHandle->document, desc, obj) == false)
	{
        mHandle->DeserializeClassObject(mHandle->document, desc, obj);
	}
}

bool JSONSerializer::TryCustomObjectSerializer(const void* obj,
											   const TStringView fieldName,
											   const Reflection::ClassDescription& classDesc,
											   void* userData)
{
	auto it = mCustomObjectSerializers.find(classDesc.ResolveName());
	if (it != mCustomObjectSerializers.end())
	{
		it->second(obj, fieldName, classDesc, userData);
		return true;
	}
	return false;
}

bool JSONSerializer::TryCustomArraySerializer(const void* obj,
											  const Reflection::ClassDescription& classDesc,
											  void* userData)
{
	auto it = mCustomArraySerializers.find(classDesc.ResolveName());
	if (it != mCustomArraySerializers.end())
	{
		it->second(obj, classDesc, userData);
		return true;
	}
	return false;
}

bool JSONSerializer::TryCustomObjectDeserializer(const void* userData,
												 const Reflection::ClassDescription& classDesc,
												 void* obj)
{
	auto it = mCustomObjectDeserializers.find(classDesc.ResolveName());
	if (it != mCustomObjectDeserializers.end())
	{
		it->second(userData, classDesc, obj);
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

void JSONSerializer::Impl::SerializeEnumObjectValue(const void* obj, rapidjson::Node& node)
{
    node.AddMember("Value", Reflection::Get<int64_t>(obj));
}

void JSONSerializer::Impl::SerializeEnumArrayValue(const void* obj, rapidjson::Node& node)
{
    node.PushBack(rapidjson::Value(Reflection::Get<int64_t>(obj)));
}

void JSONSerializer::Impl::SerializeClassObjectFields(const void* obj,
                                                      const TStringView fieldName,
                                                      const Reflection::ClassDescription& classDesc,
                                                      rapidjson::Node& outFields)
{
    for (const auto& baseClass : classDesc.ResolveBaseClasses())
    {
        SerializeClassObjectFields(obj, fieldName, baseClass, outFields);
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
                    if (JSONSerializer::TryCustomObjectSerializer(OffsetPointer(obj, classField.offset), field.ResolveName(), fieldDesc, &fieldNode) == false)
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
                    auto elements = rapidjson::Value(rapidjson::kArrayType);
                    auto elementsNode = rapidjson::Node(elements, outFields.allocator);
					const auto& classField = field.GetField<Reflection::ClassField>();
					const auto& fieldDesc = Reflection::GetClass(classField.hash);
                    if (JSONSerializer::TryCustomArraySerializer(OffsetPointer(obj, classField.offset), fieldDesc, &elementsNode) == false)
                    {
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
            SerializeEnumArrayValue(OffsetPointer(obj, elementOffset), outElements);
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
    SerializeEnumObjectValue(obj, outObject);
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

void JSONSerializer::Impl::DeserializePrimitiveObject(const rapidjson::Value& object,
                                                      Reflection::PrimitiveType type,
                                                      void* obj)
{
	switch (type)
	{
		case Reflection::PrimitiveType::Bool:
			DeserializeValue<bool>(object, obj);
			break;
		case Reflection::PrimitiveType::Int8:
			DeserializeValue<int8_t>(object, obj);
			break;
		case Reflection::PrimitiveType::WChar:
			DeserializeValue<wchar_t>(object, obj);
			break;
		case Reflection::PrimitiveType::Char:
			DeserializeValue<char>(object, obj);
			break;
		case Reflection::PrimitiveType::Int16:
			DeserializeValue<int16_t>(object, obj);
			break;
		case Reflection::PrimitiveType::Int32:
			DeserializeValue<int32_t>(object, obj);
			break;
		case Reflection::PrimitiveType::Int64:
			DeserializeValue<int64_t>(object, obj);
			break;
		case Reflection::PrimitiveType::UInt8:
			DeserializeValue<uint8_t>(object, obj);
			break;
		case Reflection::PrimitiveType::UInt16:
			DeserializeValue<uint16_t>(object, obj);
			break;
		case Reflection::PrimitiveType::UInt32:
			DeserializeValue<uint32_t>(object, obj);
			break;
		case Reflection::PrimitiveType::UInt64:
			DeserializeValue<uint64_t>(object, obj);
			break;
		case Reflection::PrimitiveType::Float:
			DeserializeValue<float>(object, obj);
			break;
		case Reflection::PrimitiveType::Double:
			DeserializeValue<double>(object, obj);
			break;
		default:
			GLEAM_ASSERT(false, "JSONSerializer: Unknown primitive type");
			break;
	}
}

void JSONSerializer::Impl::DeserializeEnumObject(const rapidjson::Value& object,
                                                 const Reflection::EnumDescription& enumDesc,
                                                 void* obj)
{
	DeserializeValue<int64_t>(object, obj);
}

void JSONSerializer::Impl::DeserializeClassObject(const rapidjson::Value& object,
                                                  const Reflection::ClassDescription& classDesc,
                                                  void* obj)
{
    if (object.HasMember("Fields") == false)
    {
        return;
    }
    
	uint32_t fieldIdx = 0;
    auto fields = object["Fields"].GetArray();
	for (const auto& base : classDesc.ResolveBaseClasses())
	{
		const auto& field = fields[fieldIdx++];
		DeserializeClassObject(field, base, obj);
	}

	for (const auto& fieldDesc : classDesc.ResolveFields())
	{
		const auto& field = fields[fieldIdx++];
		if (fieldDesc.GetType() == Reflection::FieldType::Primitive)
		{
			const auto& primitiveField = fieldDesc.GetField<Reflection::PrimitiveField>();
			DeserializePrimitiveObject(field, primitiveField.primitive, OffsetPointer(obj, primitiveField.offset));
		}
		else if (fieldDesc.GetType() == Reflection::FieldType::Array)
		{
			const auto& arrayField = fieldDesc.GetField<Reflection::ArrayField>();
			const auto& desc = Reflection::GetArray(arrayField.hash);
			DeserializeArrayObject(field, desc, OffsetPointer(obj, arrayField.offset));
		}
		else if (fieldDesc.GetType() == Reflection::FieldType::Class)
		{
			const auto& classField = fieldDesc.GetField<Reflection::ClassField>();
			const auto& desc = Reflection::GetClass(classField.hash);
			if (JSONSerializer::TryCustomObjectDeserializer(&field, desc, OffsetPointer(obj, classField.offset)) == false)
			{
				DeserializeClassObject(field, desc, OffsetPointer(obj, classField.offset));
			}
		}
		else if (fieldDesc.GetType() == Reflection::FieldType::Enum)
		{
			const auto& enumField = fieldDesc.GetField<Reflection::EnumField>();
			const auto& desc = Reflection::GetEnum(enumField.hash);
			DeserializeEnumObject(field, desc, OffsetPointer(obj, enumField.offset));
		}
		else
		{
			GLEAM_ASSERT(false, "JSONSerializer: Unknown object kind");
			continue;
		}
	}
}

void JSONSerializer::Impl::DeserializeArrayObject(const rapidjson::Value& object,
                                                  const Reflection::ArrayDescription& arrayDesc,
                                                  void* obj)
{
    if (object.HasMember("Elements") == false)
    {
        return;
    }
    
	switch (arrayDesc.ElementType())
	{
		case Reflection::FieldType::Primitive:
		{
			auto primitiveType = Reflection::Database::GetPrimitiveType(arrayDesc.ElementHash());

			size_t offset = 0;
			for (const auto& element : object["Elements"].GetArray())
			{
				DeserializePrimitiveObject(element, primitiveType, OffsetPointer(obj, offset));
				offset += arrayDesc.GetStride();
			}
			return;
		}
		case Reflection::FieldType::Array:
		{
			const auto& innerDesc = Reflection::GetArray(arrayDesc.ElementHash());

			size_t offset = 0;
			for (const auto& element : object["Elements"].GetArray())
			{
				DeserializeArrayObject(element, innerDesc, OffsetPointer(obj, offset));
				offset += arrayDesc.GetStride();
			}
			return;
		}
		case Reflection::FieldType::Class:
		{
			const auto& classDesc = Reflection::GetClass(arrayDesc.ElementHash());

			size_t offset = 0;
			for (const auto& element : object["Elements"].GetArray())
			{
				if (JSONSerializer::TryCustomObjectDeserializer(&element, classDesc, OffsetPointer(obj, offset)) == false)
				{
					DeserializeClassObject(element, classDesc, OffsetPointer(obj, offset));
				}
				offset += arrayDesc.GetStride();
			}
			return;
		}
		case Reflection::FieldType::Enum:
		{
			const auto& enumDesc = Reflection::GetEnum(arrayDesc.ElementHash());

			size_t offset = 0;
			for (const auto& element : object["Elements"].GetArray())
			{
				DeserializeEnumObject(element, enumDesc, OffsetPointer(obj, offset));
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
