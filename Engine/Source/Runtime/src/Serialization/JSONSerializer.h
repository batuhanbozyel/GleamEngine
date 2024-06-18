#pragma once
#include "Core/Subsystem.h"

namespace Gleam {

class JSONSerializer final : public Subsystem
{
public:
    
    JSONSerializer() = default;
    
    JSONSerializer(std::iostream& stream);
    
    ~JSONSerializer();
    
    virtual void Initialize() override;
    
    virtual void Shutdown() override;

    template<typename T>
    void Serialize(const T& object)
    {
        const auto& classDesc = Reflection::GetClass<T>();
        Serialize(&object, classDesc);
    }

	template<typename T>
	T Deserialize()
	{
		T object = T();
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
    
private:
    
    struct Impl;
    Impl* mHandle = nullptr;
    
};

} // namespace Gleam
