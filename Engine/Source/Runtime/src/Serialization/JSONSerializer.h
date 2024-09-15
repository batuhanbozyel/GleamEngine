#pragma once
#include "Core/Subsystem.h"

namespace Gleam {

struct JSONHeader
{
	Reflection::FieldType kind = Reflection::FieldType::Invalid;
	TString name = "";
	Guid guid = Guid::InvalidGuid();
	uint32_t version = 0;
};

class JSONSerializer final : public EngineSubsystem
{
public:
    
    JSONSerializer() = default;
    
    JSONSerializer(FileStream& stream);
    
    ~JSONSerializer();
    
    virtual void Initialize(Engine* engine) override;
    
    virtual void Shutdown() override;

	JSONHeader ParseHeader();

    template<typename T>
    void Serialize(const T& object)
    {
        const auto& classDesc = Reflection::GetClass<T>();
        Serialize(&object, classDesc);
    }

	template<typename T>
	T Deserialize()
	{
		T object{};
		const auto& classDesc = Reflection::GetClass<T>();
		Deserialize(classDesc, &object);
		return object;
	}

    void Serialize(const void* obj, const Reflection::ClassDescription& desc);
    
	void Deserialize(const Reflection::ClassDescription& desc, void* obj);
    
    static bool TryCustomObjectSerializer(const void* obj,
                                          const TStringView fieldName,
                                          const Reflection::ClassDescription& classDesc,
                                          void* userData);
    
    static bool TryCustomArraySerializer(const void* obj,
                                         const Reflection::ClassDescription& classDesc,
                                         void* userData);

	static bool TryCustomObjectDeserializer(const void* userData,
											const Reflection::ClassDescription& classDesc,
											void* obj);
    
    static bool TryCustomArrayDeserializer(const void* userData,
                                           const Reflection::ClassDescription& classDesc,
                                           void* obj);
    
private:
    
    using ObjectSerializerFn = std::function<void(const void* obj,
                                                  const TStringView fieldName,
                                                  const Reflection::ClassDescription& classDesc,
                                                  void* userData)>;
    
    using ArraySerializerFn = std::function<void(const void* obj,
                                                 const Reflection::ClassDescription& classDesc,
                                                 void* userData)>;

	using ObjectDeserializerFn = std::function<void(const void* userData,
													const Reflection::ClassDescription& classDesc,
													void* obj)>;

	using ArrayDeserializerFn = std::function<void(const void* userData,
												   const Reflection::ClassDescription& classDesc,
												   void* obj)>;

    static inline HashMap<TStringView, ObjectSerializerFn> mCustomObjectSerializers;
    
    static inline HashMap<TStringView, ArraySerializerFn> mCustomArraySerializers;

	static inline HashMap<TStringView, ObjectDeserializerFn> mCustomObjectDeserializers;
    
    static inline HashMap<TStringView, ArrayDeserializerFn> mCustomArrayDeserializers;
    
private:
    
    struct Impl;
    Impl* mHandle = nullptr;
    
};

} // namespace Gleam
