#pragma once
#include "Core/Subsystem.h"
#include "Core/Macro.h"
#include "Core/GUID.h"

#include "Container/String.h"
#include "Container/Hash.h"
#include "IO/Filesystem.h"

#include <Reflection/Reflection.h>
#ifndef __GLEAM_REFLECTION__
#include <Runtime.Reflection.generated.h>
#endif

namespace rapidjson {
struct Node;
struct ConstNode;
} // namespace rapidjson

namespace Gleam {

struct JSONHeader
{
	Reflection::MetaType kind = Reflection::MetaType::Invalid;
	Guid guid = Guid::InvalidGuid();
	uint32_t version = 0;
};

class JSONSerializer final : public EngineSubsystem
{
public:
    
    virtual void Initialize(Engine* engine) override;
    
    virtual void Shutdown(Engine* engine) override;

	JSONHeader ParseHeader(FileStream& stream);

    template<typename T>
    void Serialize(const T& object, FileStream& stream)
    {
        const auto& classDesc = Reflection::GetClass<T>();
        Serialize(&object, classDesc, stream);
    }

	template<typename T>
	void Serialize(const T& object, rapidjson::Node& root)
	{
		const auto& classDesc = Reflection::GetClass<T>();
		Serialize(&object, classDesc, root);
	}

	template<typename T>
	T Deserialize(FileStream& stream)
	{
		T object{};
		const auto& classDesc = Reflection::GetClass<T>();
		Deserialize(stream, classDesc, &object);
		return object;
	}

	template<typename T>
	T Deserialize(const rapidjson::ConstNode& root)
	{
		T object{};
		const auto& classDesc = Reflection::GetClass<T>();
		Deserialize(classDesc, &object, root);
		return object;
	}

    void Serialize(const void* obj, const Reflection::ClassDescription& classDesc, FileStream& stream);

	void Serialize(const void* obj, const Reflection::ClassDescription& classDesc, rapidjson::Node& root);

	TString Serialize(const void* obj, const Reflection::ClassDescription& classDesc);
    
	void Deserialize(FileStream& stream, const Reflection::ClassDescription& classDesc, void* obj);

	void Deserialize(const Reflection::ClassDescription& classDesc, void* obj, const rapidjson::ConstNode& root);

	void Deserialize(const Reflection::ClassDescription& classDesc, void* obj, const TString& data);

    static bool TryCustomObjectSerializer(const void* obj,
                                          const Reflection::ClassDescription& classDesc,
                                          rapidjson::Node& node);
    
    static bool TryCustomArraySerializer(const void* obj,
                                         const Reflection::ClassDescription& classDesc,
										 rapidjson::Node& node);

	static bool TryCustomObjectDeserializer(const rapidjson::ConstNode& node,
											const Reflection::ClassDescription& classDesc,
											void* obj);
    
    static bool TryCustomArrayDeserializer(const rapidjson::ConstNode& node,
                                           const Reflection::ClassDescription& classDesc,
                                           void* obj);
    
private:
    
    using ObjectSerializerFn = std::function<void(const void* obj,
                                                  const Reflection::ClassDescription& classDesc,
												  rapidjson::Node& node)>;
    
    using ArraySerializerFn = std::function<void(const void* obj,
                                                 const Reflection::ClassDescription& classDesc,
												 rapidjson::Node& node)>;

	using ObjectDeserializerFn = std::function<void(const rapidjson::ConstNode& node,
													const Reflection::ClassDescription& classDesc,
													void* obj)>;

	using ArrayDeserializerFn = std::function<void(const rapidjson::ConstNode& node,
												   const Reflection::ClassDescription& classDesc,
												   void* obj)>;

    static inline HashMap<TStringView, ObjectSerializerFn> mCustomObjectSerializers;
    
    static inline HashMap<TStringView, ArraySerializerFn> mCustomArraySerializers;

	static inline HashMap<TStringView, ObjectDeserializerFn> mCustomObjectDeserializers;
    
    static inline HashMap<TStringView, ArrayDeserializerFn> mCustomArrayDeserializers;
    
};

} // namespace Gleam

#define REGISTER_VECTOR_TYPE_JSON_SERIALIZER(Type, ...) \
if constexpr (Reflection::Traits::IsReflected<Type>()) \
{ \
    const auto qualifiedName = QualifiedNameWithoutTemplateDeclaration(Reflection::GetClass<Type>().ResolveQualifiedName()); \
    mCustomObjectSerializers[qualifiedName] = [](const void* obj, \
        const Reflection::ClassDescription& classDesc, \
        rapidjson::Node& node) \
    { \
        const auto& val = Reflection::Get<Type>(obj); \
        rapidjson::Value value(rapidjson::kObjectType); \
        GLEAM_FOREACH(SERIALIZE_MEMBER, __VA_ARGS__) \
        SerializeClassHeader(classDesc, node); \
        node.AddMember("Value", value); \
    }; \
    mCustomArraySerializers[qualifiedName] = [](const void* obj, \
        const Reflection::ClassDescription& classDesc, \
        rapidjson::Node& node) \
    { \
        const auto& val = Reflection::Get<Type>(obj); \
        rapidjson::Value value(rapidjson::kObjectType); \
        GLEAM_FOREACH(SERIALIZE_MEMBER, __VA_ARGS__) \
        node.PushBack(rapidjson::Value(value, node.allocator)); \
    }; \
    mCustomObjectDeserializers[qualifiedName] = [](const rapidjson::ConstNode& node, \
        const Reflection::ClassDescription& classDesc, \
        void* obj) \
    { \
        if (node.object.HasMember("Value")) \
        { \
            const auto& value = node.object["Value"].GetObj(); \
            auto& val = Reflection::Get<Type>(obj); \
            GLEAM_FOREACH(DESERIALIZE_MEMBER, __VA_ARGS__) \
        } \
    }; \
    mCustomArrayDeserializers[qualifiedName] = [](const rapidjson::ConstNode& node, \
        const Reflection::ClassDescription& classDesc, \
        void* obj) \
    { \
        const auto& value = node.object.GetObj(); \
        auto& val = Reflection::Get<Type>(obj); \
        GLEAM_FOREACH(DESERIALIZE_MEMBER, __VA_ARGS__) \
    }; \
}

// Helper macros for member serialization
#define SERIALIZE_MEMBER(member) \
    value.AddMember(GLEAM_STRINGIFY(member), val.member, node.allocator);

#define DESERIALIZE_MEMBER(member) \
    if constexpr (std::is_floating_point_v<decltype(val.member)>) { \
        val.member = value[GLEAM_STRINGIFY(member)].GetFloat(); \
    } else if constexpr (std::is_integral_v<decltype(val.member)>) { \
        val.member = value[GLEAM_STRINGIFY(member)].GetInt(); \
    }

#define REGISTER_MATRIX_TYPE_JSON_SERIALIZER(Type, Size) \
if constexpr (Reflection::Traits::IsReflected<Type>()) \
{ \
    const auto qualifiedName = Reflection::GetClass<Type>().ResolveQualifiedName(); \
    mCustomObjectSerializers[qualifiedName] = [](const void* obj, \
        const Reflection::ClassDescription& classDesc, \
        rapidjson::Node& node) \
    { \
        const auto& mat = Reflection::Get<Type>(obj); \
        rapidjson::Value value(rapidjson::kArrayType); \
        for (size_t i = 0; i < Size; ++i) \
        { \
            value.PushBack(mat.m[i], node.allocator); \
        } \
        SerializeClassHeader(classDesc, node); \
        node.AddMember("Value", value); \
    }; \
    mCustomArraySerializers[qualifiedName] = [](const void* obj, \
        const Reflection::ClassDescription& classDesc, \
        rapidjson::Node& node) \
    { \
        const auto& mat = Reflection::Get<Type>(obj); \
        rapidjson::Value value(rapidjson::kArrayType); \
        for (size_t i = 0; i < Size; ++i) \
        { \
            value.PushBack(mat.m[i], node.allocator); \
        } \
        node.PushBack(rapidjson::Value(value, node.allocator)); \
    }; \
    mCustomObjectDeserializers[qualifiedName] = [](const rapidjson::ConstNode& node, \
        const Reflection::ClassDescription& classDesc, \
        void* obj) \
    { \
        if (node.object.HasMember("Value")) \
        { \
            const auto& value = node.object["Value"].GetArray(); \
            auto& mat = Reflection::Get<Type>(obj); \
            for (size_t i = 0; i < Size; ++i) \
            { \
				if constexpr (std::is_floating_point_v<decltype(mat.m[i])>) \
					mat.m[i] = value[i].GetFloat(); \
				else if constexpr (std::is_integral_v<decltype(mat.m[i])>) \
					mat.m[i] = value[i].GetInt(); \
            } \
        } \
    }; \
    mCustomArrayDeserializers[qualifiedName] = [](const rapidjson::ConstNode& node, \
        const Reflection::ClassDescription& classDesc, \
        void* obj) \
    { \
        const auto& value = node.object.GetArray(); \
        auto& mat = Reflection::Get<Type>(obj); \
        for (size_t i = 0; i < Size; ++i) \
        { \
            if constexpr (std::is_floating_point_v<decltype(mat.m[i])>) \
				mat.m[i] = value[i].GetFloat(); \
			else if constexpr (std::is_integral_v<decltype(mat.m[i])>) \
				mat.m[i] = value[i].GetInt(); \
        } \
    }; \
}
